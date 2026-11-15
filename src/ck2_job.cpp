// ---------------------------------------------------------------------------
// CorridorKey 2 for Resolve - runs the Python worker and follows its progress
//
// From Creative Video Tips (CVT)  |  https://creativevideotips.com
// Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
// ---------------------------------------------------------------------------
#include "ck2_job.h"
#include "ck2_core.h"

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace ck2 {

namespace {
// "2 chunks of 80" / "1 chunk of 23"
std::string chunksText(const std::string& chunks, const std::string& window) {
    return chunks + (chunks == "1" ? " chunk of " : " chunks of ") + window;
}
}  // namespace

std::shared_ptr<Job> Job::start(const std::string& python, const std::string& worker,
                                const std::string& jobPath, const std::string& logPath,
                                Callback onUpdate, std::string& error) {
    int fds[2];
    if (pipe(fds) != 0) { error = "pipe failed"; return nullptr; }

    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0);
    posix_spawn_file_actions_adddup2(&fa, fds[1], 1);
    posix_spawn_file_actions_addopen(&fa, 2, logPath.c_str(),
                                     O_WRONLY | O_CREAT | O_APPEND, 0600);
    posix_spawn_file_actions_addclose(&fa, fds[0]);
    posix_spawn_file_actions_addclose(&fa, fds[1]);

    std::vector<std::string> args = {python, "-u", worker, "run", jobPath};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);

    // A small, clean environment: the host's own Python settings must not leak in.
    const char* home = std::getenv("HOME");
    std::vector<std::string> env = {
        std::string("HOME=") + (home ? home : ""),
        "PATH=/usr/bin:/bin:/usr/sbin:/sbin",
        "LANG=en_US.UTF-8",
        "PYTHONNOUSERSITE=1",
    };
    // The progress window needs the desktop session; CUDA_VISIBLE_DEVICES picks the GPU.
    for (const char* key : {"TMPDIR", "DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY", "XDG_RUNTIME_DIR",
                            "CUDA_VISIBLE_DEVICES"})
        if (const char* v = std::getenv(key)) env.push_back(std::string(key) + "=" + v);
    std::vector<char*> envp;
    for (auto& e : env) envp.push_back(e.data());
    envp.push_back(nullptr);

    pid_t pid = -1;
    int rc = posix_spawn(&pid, python.c_str(), &fa, nullptr, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&fa);
    close(fds[1]);
    if (rc != 0) {
        close(fds[0]);
        error = std::string("could not start Python: ") + std::strerror(rc);
        return nullptr;
    }

    auto job = std::shared_ptr<Job>(new Job());
    job->callback_ = std::move(onUpdate);
    job->pid_ = pid;
    job->running_ = true;
    std::thread([job, fd = fds[0]] { job->readLoop(fd); }).detach();
    return job;
}

bool Job::running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void Job::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_ && pid_ > 0) {
        cancelled_ = true;
        kill(pid_, SIGTERM);
    }
}

void Job::detach() {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = nullptr;
}

void Job::report(const JobUpdate& u) {
    // Held during the call so detach() waits until no callback is running.
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_) callback_(u);
}

void Job::readLoop(int fd) {
    std::string pending, lastError, doneText;
    char buf[4096];
    while (true) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        pending.append(buf, static_cast<size_t>(n));
        size_t nl;
        while ((nl = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, nl);
            pending.erase(0, nl + 1);
            std::string ev = jsonField(line, "event");
            JobUpdate u;
            if (ev == "plan") {
                u.status = "Starting: " + jsonField(line, "frames") + " frames in " +
                           chunksText(jsonField(line, "chunks"), jsonField(line, "window"));
            } else if (ev == "status") {
                std::string m = jsonField(line, "message");
                u.status = m.empty() ? "" : std::string(1, static_cast<char>(std::toupper(m[0]))) + m.substr(1);
            } else if (ev == "progress") {
                u.status = "Keying frame " + jsonField(line, "done") + " of " + jsonField(line, "total") +
                           " (chunk " + jsonField(line, "chunk") + " of " + jsonField(line, "chunks") + ")";
            } else if (ev == "done") {
                doneText = "Done: " + jsonField(line, "frames") + " frames in " +
                           jsonField(line, "seconds") + " s (" +
                           chunksText(jsonField(line, "chunks"), jsonField(line, "window")) + " frames)";
                continue;
            } else if (ev == "error") {
                lastError = jsonField(line, "message");
                continue;
            }
            if (!u.status.empty()) report(u);
        }
    }
    close(fd);

    int st = 0;
    while (waitpid(pid_, &st, 0) < 0 && errno == EINTR) {}
    bool cancelled;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        cancelled = cancelled_;
    }
    JobUpdate u;
    u.finished = true;
    u.ok = WIFEXITED(st) && WEXITSTATUS(st) == 0 && !doneText.empty();
    if (u.ok) u.status = doneText;
    else if (cancelled || lastError == "Cancelled") u.status = "Stopped";
    else if (!lastError.empty()) u.status = "Error: " + lastError;
    else u.status = "Error: the worker stopped (see the log)";
    report(u);
}

}  // namespace ck2

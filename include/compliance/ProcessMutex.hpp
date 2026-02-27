#pragma once
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#endif

class ProcessMutex
{
public:
    ProcessMutex(const std::string& name) : name_(name), locked_(false)
    {
#ifdef _WIN32
        mutex_ = CreateMutexA(NULL, FALSE, name.c_str());
        if (mutex_ == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
            locked_ = false;
        } else {
            WaitForSingleObject(mutex_, INFINITE);
            locked_ = true;
        }
#else
        lockfile_ = "/tmp/" + name + ".lock";
        fd_ = open(lockfile_.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd_ != -1) {
            if (flock(fd_, LOCK_EX | LOCK_NB) == 0) {
                locked_ = true;
            }
        }
#endif
    }
    
    ~ProcessMutex()
    {
#ifdef _WIN32
        if (mutex_) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
        }
#else
        if (fd_ != -1) {
            flock(fd_, LOCK_UN);
            close(fd_);
        }
#endif
    }
    
    bool isLocked() const { return locked_; }

private:
    std::string name_;
    bool locked_;
    
#ifdef _WIN32
    HANDLE mutex_;
#else
    std::string lockfile_;
    int fd_;
#endif
};

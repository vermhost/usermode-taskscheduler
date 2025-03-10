#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cstdio>
#include <tlhelp32.h>
#include <memory>

namespace task_scheduler {
    namespace offsets {
        static uintptr_t task_scheduler_ptr = 0x5E723A8;
        static uintptr_t job_name = 0x90;

        static uintptr_t children_offset = 0x70;
        static uintptr_t childrensize_offset = 0x8;
        static uintptr_t parent_offset = 0x40;
        static uintptr_t name_offset = 0x68;
        static uintptr_t ClassName_offset = 0x8;
        static uintptr_t ClassDescriptor_offset = 0x18;

        namespace renderjob {
            static uintptr_t renderview_ptr = 0x218;
            static uintptr_t visualengine_ptr = 0x10;

            static uintptr_t datamodel_ptr = 0xB0;
            static uintptr_t datamodel_offset = 0x190;
        }
    }

    inline std::string wchar_to_string(const WCHAR* wchar_array) {
        if (!wchar_array) return "";

        int buffer_size = WideCharToMultiByte(CP_UTF8, 0, wchar_array, -1, nullptr, 0, nullptr, nullptr);
        if (buffer_size == 0) {
            printf("Error: Failed to convert wide string. Error Code: %lu\n", GetLastError());
            return "";
        }

        std::vector<char> buffer(buffer_size);
        if (WideCharToMultiByte(CP_UTF8, 0, wchar_array, -1, buffer.data(), buffer_size, nullptr, nullptr) == 0) {
            printf("Error: Failed to convert wide string. Error Code: %lu\n", GetLastError());
            return "";
        }

        return std::string(buffer.data());
    }

    inline DWORD get_roblox_process_id() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            printf("Error: Failed to create process snapshot. Error Code: %lu\n", GetLastError());
            return 0;
        }

        PROCESSENTRY32 process_entry;
        process_entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &process_entry)) {
            do {
                if (wchar_to_string(process_entry.szExeFile) == "RobloxPlayerBeta.exe") {
                    CloseHandle(snapshot);
                    return process_entry.th32ProcessID;
                }
            } while (Process32Next(snapshot, &process_entry));
        }

        CloseHandle(snapshot);
        printf("[ERROR] RobloxPlayerBeta.exe process not found.\n");
        return 0;
    }

    inline uintptr_t get_base_address(DWORD process_id) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_id);
        if (snapshot == INVALID_HANDLE_VALUE) {
            printf("Error: Failed to create module snapshot. Error Code: %lu\n", GetLastError());
            return 0;
        }

        MODULEENTRY32 module_entry;
        module_entry.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(snapshot, &module_entry)) {
            do {
                if (wchar_to_string(module_entry.szModule) == "RobloxPlayerBeta.exe") {
                    CloseHandle(snapshot);
                    return reinterpret_cast<uintptr_t>(module_entry.modBaseAddr);
                }
            } while (Module32Next(snapshot, &module_entry));
        }

        CloseHandle(snapshot);
        printf("Error: Failed to find base address of RobloxPlayerBeta.exe.\n");
        return 0;
    }

    template <typename T>
    inline T read_memory(HANDLE process_handle, uintptr_t address) {
        T value;
        if (!ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr)) {
            return T();
        }
        return value;
    }

    inline uintptr_t get_scheduler(HANDLE process_handle, uintptr_t base_address) {
        uintptr_t scheduler_ptr = base_address + offsets::task_scheduler_ptr;
        uintptr_t scheduler = read_memory<uintptr_t>(process_handle, scheduler_ptr);
        return scheduler;
    }

    inline std::string get_job_name(HANDLE process_handle, uintptr_t job) {
        uintptr_t to_read = job + offsets::job_name;
        const int length = read_memory<int>(process_handle, to_read + 0x18);

        if (length >= 16) {
            to_read = read_memory<uintptr_t>(process_handle, to_read);
        }

        std::vector<char> buffer(256);
        if (!ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(to_read), buffer.data(), buffer.size(), nullptr)) {
            return "";
        }

        return std::string(buffer.data());
    }

    inline int get_total_jobs(HANDLE process_handle, uintptr_t task_scheduler) {
        if (task_scheduler == 0x0) {
            printf("[ERROR] Task Scheduler is null.\n");
            return 0;
        }

        uint64_t scheduler_metadata = read_memory<uint64_t>(process_handle, task_scheduler - sizeof(uint64_t));
        int total_jobs = (scheduler_metadata >> 54) & 0x3F;
        return total_jobs;
    }

    inline std::vector<uintptr_t> active_jobs(HANDLE process_handle, uintptr_t task_scheduler) {
        std::vector<uintptr_t> jobs;

        if (task_scheduler == 0x0) {
            printf("[ERROR] Task Scheduler is null.\n");
            return jobs;
        }

        int total_jobs = get_total_jobs(process_handle, task_scheduler);

        for (uintptr_t job = read_memory<uintptr_t>(process_handle, task_scheduler), job_index = 0;
            job_index <= total_jobs && job != 0x0;
            job_index += 1, job = read_memory<uintptr_t>(process_handle, task_scheduler + (job_index * sizeof(uintptr_t)))) {

            if (get_job_name(process_handle, job).empty())
                continue;

            jobs.push_back(job);
        }

        return jobs;
    }

    inline void debug_print_all_jobs(HANDLE process_handle, uintptr_t task_scheduler) {
        auto jobs = active_jobs(process_handle, task_scheduler);
        for (const auto& job : jobs) {
            std::string job_name = get_job_name(process_handle, job);
            printf("[DEBUG] Job: 0x%llx, Name: %s\n", job, job_name.c_str());
        }
    }

    inline std::vector<uintptr_t> get_jobs(HANDLE process_handle, uintptr_t task_scheduler, const std::string& name) {
        std::vector<uintptr_t> result;

        for (const auto& job : active_jobs(process_handle, task_scheduler)) {
            if (get_job_name(process_handle, job) == name)
                result.push_back(job);
        }

        return result;
    }

    inline uintptr_t get_job(HANDLE process_handle, uintptr_t task_scheduler, const std::string& name) {
        for (const auto& job : active_jobs(process_handle, task_scheduler)) {
            if (get_job_name(process_handle, job) == name)
                return job;
        }

        return 0x0;
    }

    inline uintptr_t get_renderview(HANDLE process_handle, uintptr_t task_scheduler) {
        const uintptr_t render_job = get_job(process_handle, task_scheduler, "RenderJob");
        if (render_job == 0x0) {
            printf("[ERROR] RenderJob not found.\n");
            return 0x0;
        }

        return read_memory<uintptr_t>(process_handle, render_job + offsets::renderjob::renderview_ptr);
    }

    inline uintptr_t get_datamodel(HANDLE process_handle, uintptr_t task_scheduler) {
        const uintptr_t render_job = get_job(process_handle, task_scheduler, "RenderJob");
        if (render_job == 0x0) {
            printf("[ERROR] RenderJob not found. - .hpp\n");
            return 0x0;
        }

        return read_memory<uintptr_t>(process_handle, render_job + offsets::renderjob::datamodel_ptr) + offsets::renderjob::datamodel_offset;
    }

    inline std::string get_datamodel_name(HANDLE process_handle, uintptr_t datamodel) {
        uintptr_t name_ptr = read_memory<uintptr_t>(process_handle, datamodel + offsets::name_offset);
        std::vector<char> buffer(256);

        if (!ReadProcessMemory(process_handle, reinterpret_cast<LPCVOID>(name_ptr), buffer.data(), buffer.size(), nullptr)) {
            printf("[ERROR] Failed to read DataModel name. Error Code: %lu\n", GetLastError());
            return "";
        }

        return std::string(buffer.data());
    }
}

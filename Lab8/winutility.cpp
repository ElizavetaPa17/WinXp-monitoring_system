#define _WIN32_WINNT 0x0602
#define SystemProcessorPerfomanceInformation 0x8
#define SystemBasicInformation

#include "winutility.h"

#include <stdio.h>
#include "QTimer"
#include "QDebug"

const int MEMORYSTATUSEX_MEMBER_COUNT = 2;

WinUtility* WinUtility::singleton_ = NULL;

WinUtility::WinUtility(QObject* parent)
    : QObject(parent)
{
    hinst_lib_ = LoadLibraryA("C:\\programs\\lab8_dll.dll");
    if (hinst_lib_ == NULL) {
        qDebug() << "LoadLibrary() failed" << GetLastError();
        printf("%p\n%d\n%d\n",  hinst_lib_, GetLastError(), sizeof(int*));
        abort();
    }

    setupImportFunctions();
}

WinUtility::~WinUtility(){
    FreeLibrary(hinst_lib_);
}

WinUtility* WinUtility::getInstance() {
    if (singleton_ == NULL) {
        singleton_ = new WinUtility(NULL);
    }

    return singleton_;
}

std::vector<std::string> WinUtility::getAllProccessName() {
    return processes_names_;
}

std::vector<double> WinUtility::getAllProcessCPUUsage() {
    return processes_cpu_usage_;
}

std::vector<double> WinUtility::getAllProccessMemoryUsage() {
    return processes_mem_usage_;
}

std::vector<double> WinUtility::getAllProcessDiskActivity() {
    return processes_disk_act_;
}

void WinUtility::slotSetProcessInfoReady() {
    double next_process_time;
    FILETIME next_sys_kernel_time, next_sys_user_time, plug;
    getTotalSystemTimes(next_sys_kernel_time, next_sys_user_time, plug);
    ULONGLONG next_sys_time = sumTwoFileTimes(next_sys_kernel_time, next_sys_user_time);

    processes_cpu_usage_.resize(h_processes_.size());
    for (unsigned int i = 0; i < h_processes_.size(); ++i) {
        if (getTotalProcessTimes(h_processes_[i], next_process_time) == FALSE) {
            processes_cpu_usage_[i] = -1;
        } else {
            double cpu_usage = ((double(next_process_time) - processes_user_time_[i]) / (next_sys_time - sys_time_)) * 100;
            processes_cpu_usage_[i] = cpu_usage;
        }
    }

    IO_COUNTERS io_count;
    processes_disk_act_.resize(h_processes_.size());
    for (unsigned int i = 0; i < h_processes_.size(); ++i) {
        if (GetProcessIoCounters(h_processes_[i], &io_count) == FALSE) {
            qDebug() << "GetProcessIoCounters() failed:" << GetLastError();
            processes_disk_act_[i] = -1;
        } else {
            ULONGLONG next_total_proc_oper_count = io_count.ReadTransferCount + io_count.WriteTransferCount + io_count.ReadTransferCount;
            processes_disk_act_[i] = double(next_total_proc_oper_count - total_proc_disk_oper_count_[i]) / 1000;
        }
    }

    emit signalProcessInfoReady();
    closeProcesses();
}

void WinUtility::slotUpdateProcessInfo() {
    updateAllProcessHandle();
    updateAllProccessName();
    updateAllProcessCPUUsage();
    updateAllProcessDiskActivity();
    updateAllProccessMemoryUsage();

    QTimer::singleShot(1000, this, SLOT(slotSetProcessInfoReady()));
}

void WinUtility::updateAllProcessHandle() {
    DWORD a_process[1024];
    DWORD bytes_handled;

    if (!EnumProcesses(a_process, sizeof(a_process), &bytes_handled)) {
        qDebug() << "EnumProccess failed(): " << GetLastError();
    } else {
        int count_process = bytes_handled / sizeof(DWORD);

        h_processes_.clear();
        for(int i = 0; i < count_process; ++i) {
            HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, a_process[i]);
            if (h_process != NULL) {
                h_processes_.push_back(h_process);
            }
        }
    }
}

void WinUtility::updateAllProccessName() {
    char buff[200];
    processes_names_.resize(h_processes_.size());
    unsigned int slash_index;

    for (unsigned int i = 0; i < h_processes_.size(); ++i) {
        GetProcessImageFileNameA(h_processes_[i], buff, 200);
        processes_names_[i].insert(0, buff);

        slash_index = processes_names_[i].find_last_of('\\');
        if (slash_index != (unsigned int) -1) {
            processes_names_[i] = processes_names_[i].substr(slash_index+1, processes_names_[i].size() - slash_index);
        } else {
            processes_names_[i].clear();
            processes_names_[i] = (QString("sys_proc") + QString::number(i)).toStdString();
        }
    }
}

void WinUtility::updateAllProcessCPUUsage() {
    processes_user_time_.resize(h_processes_.size());

    FILETIME sys_kernel_time, sys_user_time;
    GetSystemTimes(&idle_time_, &sys_kernel_time, &sys_user_time);
    sys_time_ = sumTwoFileTimes(sys_kernel_time, sys_user_time);

    for (unsigned int i = 0; i < h_processes_.size(); ++i) {
        getTotalProcessTimes(h_processes_[i], processes_user_time_[i]);
    }
}

void WinUtility::updateAllProccessMemoryUsage() {
    processes_mem_usage_.resize(h_processes_.size());
    MEMORYSTATUSEX memstatus = { sizeof(memstatus) };

    if (GlobalMemoryStatusEx(&memstatus) == FALSE) {
        qDebug() << "GlobalMemoryStatusEx() failed:" << GetLastError();
    }

    DWORDLONG totalPhysMem = memstatus.ullTotalPhys;
    PROCESS_MEMORY_COUNTERS proc_mem_counter;

    for (int i = 0; i < h_processes_.size(); ++i) {
        if (GetProcessMemoryInfo(h_processes_[i], &proc_mem_counter, sizeof(proc_mem_counter)) == FALSE) {
            qDebug() << "GetProcessMemoryInfo() failed:" << GetLastError();
            processes_mem_usage_[i] = -1;
        } else {
            processes_mem_usage_[i] = (double(proc_mem_counter.WorkingSetSize) / totalPhysMem) * 100;
        }
    }
}

void WinUtility::updateAllProcessDiskActivity() {
    total_proc_disk_oper_count_.resize(h_processes_.size());

    IO_COUNTERS io_count;
    for (unsigned int i = 0; i < h_processes_.size(); ++i) {
        if (GetProcessIoCounters(h_processes_[i], &io_count) == FALSE) {
            qDebug() << "GetProcessIoCounters() failed:" << GetLastError();
            total_proc_disk_oper_count_[i] = -1;
        } else {
            total_proc_disk_oper_count_[i] = io_count.ReadTransferCount + io_count.WriteTransferCount + io_count.ReadTransferCount;
        }
    }
}

std::vector<DWORDLONG> WinUtility::getMemoryInfo() {
    MEMORYSTATUSEX mem_status = { sizeof(mem_status) };

    if (GlobalMemoryStatusEx(&mem_status) == FALSE) {
        qDebug() << "GlobalMemoryStatusEx() failed:" << GetLastError();
        return std::vector<DWORDLONG>(MEMORYSTATUSEX_MEMBER_COUNT, -1);
    } else {
        std::vector<DWORDLONG> v_memory_info(MEMORYSTATUSEX_MEMBER_COUNT);
        v_memory_info[0] = mem_status.dwMemoryLoad;
        v_memory_info[1] = mem_status.ullAvailPhys;

        return v_memory_info;
    }
}

double WinUtility::getCPUUsage() {
    double cpu_usage = 0;
    for (int i = 0; i < processes_cpu_usage_.size(); ++i) {
        cpu_usage += processes_cpu_usage_[i];
    }

    return cpu_usage;
}

//statuc functions
std::vector<std::string> WinUtility::getLogDriveStrings() {
    DWORD dw_size = MAX_PATH;
    char sz_logical_drives[MAX_PATH] = {0};
    DWORD dwResult = GetLogicalDriveStringsA(dw_size, sz_logical_drives);

    char* single_drive = sz_logical_drives;
    for (unsigned int i = 0; i < dwResult; ++i) {
        single_drive += strlen(single_drive);
    }

    return std::vector<std::string>();
}

std::string WinUtility::getFirstCPUCoreCount() {
    char buff[10];
    GetEnvironmentVariableA("NUMBER_OF_PROCESSORS", buff, sizeof(buff));

    std::string str(buff);
    return str;
}

WinUtility::ScreenResolution WinUtility::getMainScreenRes() {
    ScreenResolution screen_size;
    screen_size.width = GetSystemMetrics(SM_CXSCREEN);
    screen_size.height = GetSystemMetrics(SM_CYSCREEN);

    if (screen_size.width == 0 || screen_size.height == 0) {
        qDebug() << "GetSystemMetrics() failed: " << GetLastError();
    }

    return screen_size;
}

std::vector<DWORD> WinUtility::getOSVersion() {
    std::vector<DWORD> version;
    OSVERSIONINFOA version_info;

    version_info.dwOSVersionInfoSize = sizeof(version_info);
    GetVersionExA(&version_info);

    version.push_back(version_info.dwMajorVersion);
    version.push_back(version_info.dwMinorVersion);
    version.push_back(version_info.dwBuildNumber);

    return version;
}

std::vector<BYTE> WinUtility::getBatteryStatus() {
    std::vector<BYTE> batt_stat(3, -1);
    SYSTEM_POWER_STATUS sys_power_stat;

    if (GetSystemPowerStatus(&sys_power_stat) == FALSE) {
        qDebug() << "GetBatteryStatus() failed: " << GetLastError();
    } else {
        batt_stat[0] = sys_power_stat.ACLineStatus;
        batt_stat[1] = sys_power_stat.BatteryFlag;
        batt_stat[2] = sys_power_stat.BatteryLifePercent;
    }

    return batt_stat;
}

DWORD WinUtility::getBatteryLifeTime() {
    SYSTEM_POWER_STATUS sys_power_stat;
    if (GetSystemPowerStatus(&sys_power_stat) == FALSE) {
        qDebug() << "GetBatteryStatus() failed: " << GetLastError();
    }

    return sys_power_stat.BatteryLifeTime;
}

DWORD WinUtility::getBatteryFullLifeTime() {
    SYSTEM_POWER_STATUS sys_power_stat;
    if (GetSystemPowerStatus(&sys_power_stat) == FALSE) {
        qDebug() << "GetBatteryStatus() failed: " << GetLastError();
    }

    return sys_power_stat.BatteryFullLifeTime;
}

void WinUtility::setupImportFunctions() {
    getCPUName = (imp_getCPUName) GetProcAddress(hinst_lib_, "getCPUName");
    if (getCPUName == NULL) {
        qDebug() << "getCPUName() failed" << GetLastError();
        abort();
    }

    getVendorName = (imp_getVendorName) GetProcAddress(hinst_lib_, "getVendorName");
    if (getVendorName == NULL) {
        qDebug() << "getVendorName() failed" << GetLastError();
        abort();
    }

    getPageSize = (imp_getPageSize) GetProcAddress(hinst_lib_, "getPageSize");
    if (getPageSize == NULL) {
        qDebug() << "getPageSize() failed" << GetLastError();
        abort();
    }

    getCPUCount = (imp_getCPUCount) GetProcAddress(hinst_lib_, "getCPUCount");
    if (getCPUCount == NULL) {
        qDebug() << "getCPUCount() failed" << GetLastError();
        abort();
    }

    getVirtualAndPhysicalAddressSize = (imp_getVirtualAndPhysicalAddressSize) GetProcAddress(hinst_lib_, "getVirtualAndPhysicalAddressSize");
    if (getVirtualAndPhysicalAddressSize == NULL) {
        qDebug() << "getVirtualAndPhysicalAddressSize() failed" << GetLastError();
        abort();
    }

    getApplicationAddressBorder = (imp_getApplicationAddressBorder) GetProcAddress(hinst_lib_, "getApplicationAddressBorder");
    if (getApplicationAddressBorder == NULL) {
        qDebug() << "getApplicationAddressBorder() failed" << GetLastError();
        abort();
    }

    getDriveTypeString = (imp_getDriveTypeString) GetProcAddress(hinst_lib_, "getDriveTypeString");
    if ("getDriveTypeString" == NULL) {
        qDebug() << "getDriveTypeString() failed" << GetLastError();
        abort();
    }

    getFreeDriveSpace = (imp_getFreeDriveSpace) GetProcAddress(hinst_lib_, "getFreeDriveSpace");
    if ("getFreeDriveSpace" == NULL) {
        qDebug() << "getFreeDriveSpace() failed" << GetLastError();
        abort();
    }

    getTotalDriveSpace = (imp_getTotalDriveSpace) GetProcAddress(hinst_lib_, "getTotalDriveSpace");
    if ("getTotalDriveSpace" == NULL) {
        qDebug() << "getTotalDriveSpace() failed" << GetLastError();
        abort();
    }

    getTotalRAMSpace = (imp_getTotalRAMSpace) GetProcAddress(hinst_lib_, "getTotalRAMSpace");
    if ("getTotalRAMSpace" == NULL) {
        qDebug() << "getTotalRAMSpace() failed" << GetLastError();
        abort();
    }
}

ULONGLONG WinUtility::sumTwoFileTimes(FILETIME& lhs_time, FILETIME& rhs_time) {
    LARGE_INTEGER lhs, rhs;

    lhs.LowPart = lhs_time.dwLowDateTime;
    lhs.HighPart = lhs_time.dwHighDateTime;

    rhs.LowPart = rhs_time.dwLowDateTime;
    rhs.HighPart = rhs_time.dwHighDateTime;

    return lhs.QuadPart + rhs.QuadPart;
}

ULONGLONG WinUtility::diffTwoFileTimes(FILETIME& lhs_time, FILETIME& rhs_time) {
    LARGE_INTEGER lhs, rhs;

    lhs.LowPart = lhs_time.dwLowDateTime;
    lhs.HighPart = lhs_time.dwHighDateTime;

    rhs.LowPart = rhs_time.dwLowDateTime;
    rhs.HighPart = rhs_time.dwHighDateTime;

    return rhs.QuadPart - lhs.QuadPart;
}

BOOL WinUtility::getTotalProcessTimes(HANDLE& h_process, double& proc_time) {
    FILETIME proc_creation_time, proc_exit_time, proc_kernel_time, proc_user_time;

    if (GetProcessTimes(h_process, &proc_creation_time, &proc_exit_time, &proc_kernel_time, &proc_user_time) == FALSE) {
        qDebug() << "GetProcessTimes() failed: " << GetLastError();
        return FALSE;
    }

    proc_time = sumTwoFileTimes(proc_kernel_time, proc_user_time);
    return TRUE;
}

BOOL WinUtility::getTotalSystemTimes(FILETIME& sys_kernel_time, FILETIME& sys_user_time, FILETIME& idle_time) {
    if (GetSystemTimes(&idle_time, &sys_kernel_time, &sys_user_time) == FALSE) {
        qDebug() << "GetSystemTimes() failed: " << GetLastError();
        return FALSE;
    } else {
        return TRUE;
    }
}

void WinUtility::closeProcesses() {
    for (unsigned int i = 0; i < h_processes_.size(); ++i) {
        CloseHandle(h_processes_[i]);
    }
}

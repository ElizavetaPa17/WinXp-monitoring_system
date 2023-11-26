#ifndef WINUTILITY_H
#define WINUTILITY_H

#include <windows.h>

#include <psapi.h>
#include <vector>
#include <string>

#include "QObject"

typedef void (*imp_getCPUName) (const char*);
typedef void (*imp_getVendorName) (const char*);

typedef DWORD (*imp_getPageSize) ();
typedef DWORD (*imp_getCPUCount)();
typedef DWORD (*imp_getTotalRAMSpace) ();
typedef const char* (*imp_getDriveTypeString) (const char*);
typedef void (*imp_getVirtualAndPhysicalAddressSize) (char*, char*);
typedef void (*imp_getApplicationAddressBorder) (LPVOID*, LPVOID*);
typedef void (*imp_getFreeDriveSpace) (char*, DWORD*);
typedef void (*imp_getTotalDriveSpace) (char*, DWORD*);

class WinUtility : public QObject {
    Q_OBJECT
public:
    static WinUtility* getInstance();

    struct ScreenResolution {
        int width;
        int height;

        ScreenResolution(int width = 0, int height = 0) { this->width = width, this->height = height; }
    };

    std::vector<std::string> getAllProccessName();
    std::vector<double>      getAllProcessCPUUsage();
    std::vector<double>      getAllProccessMemoryUsage();
    std::vector<double>      getAllProcessDiskActivity();

    std::vector<DWORDLONG> getMemoryInfo();
    double getCPUUsage();

    // GET OS VERSION

    std::vector<std::string> getLogDriveStrings();
    std::string              getFirstCPUCoreCount();
    ScreenResolution         getMainScreenRes();
    std::vector<DWORD>       getOSVersion();

    std::vector<BYTE>  getBatteryStatus();
    DWORD getBatteryLifeTime();
    DWORD getBatteryFullLifeTime();

    imp_getCPUName                       getCPUName;
    imp_getVendorName                    getVendorName;
    imp_getPageSize                      getPageSize;
    imp_getCPUCount                      getCPUCount;
    imp_getDriveTypeString               getDriveTypeString;
    imp_getApplicationAddressBorder      getApplicationAddressBorder;
    imp_getVirtualAndPhysicalAddressSize getVirtualAndPhysicalAddressSize;
    imp_getFreeDriveSpace                getFreeDriveSpace;
    imp_getTotalDriveSpace               getTotalDriveSpace;
    imp_getTotalRAMSpace                 getTotalRAMSpace;

signals:
    void processInfoReady();

protected:
    WinUtility(QObject* parent = NULL);
    ~WinUtility();

private:
    WinUtility(const WinUtility&){}
    static WinUtility* singleton_;

    HINSTANCE hinst_lib_;

    std::vector<HANDLE> h_processes_;
    std::vector<double> processes_user_time_;
    std::vector<double> processes_cpu_usage_;
    std::vector<std::string> processes_names_;
    std::vector<double> processes_mem_usage_;
    std::vector<double> processes_disk_act_;
    std::vector<ULONGLONG> total_proc_disk_oper_count_;
    ULONGLONG sys_time_;
    FILETIME  idle_time_;

    void setupImportFunctions();
    void closeProcesses();

    void updateAllProcessHandle();
    void updateAllProccessName();
    void updateAllProcessCPUUsage();
    void updateAllProccessMemoryUsage();
    void updateAllProcessDiskActivity();

    ULONGLONG sumTwoFileTimes(FILETIME& rhs_time, FILETIME& lhs_time);
    ULONGLONG diffTwoFileTimes(FILETIME& rhs_time, FILETIME& lhs_time);
    BOOL getTotalSystemTimes(FILETIME& sys_kernel_time, FILETIME& sys_user_time, FILETIME& idle_time);
    BOOL getTotalProcessTimes(HANDLE& h_process, double& process_time);

signals:
    void signalProcessInfoReady();

public slots:
    void slotUpdateProcessInfo();

protected slots:
    void slotSetProcessInfoReady();
};

#endif // WINUTILITY_H

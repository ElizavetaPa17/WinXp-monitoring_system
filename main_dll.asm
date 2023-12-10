format PE GUI DLL
entry DllEntryPoint

;include 'win32ax.inc'
include 'win32wx.inc'

section '.text' code readable executable

proc DllEntryPoint hinstDLL, fdwReason, lpvReserved
  mov eax, 1                                              
  ret
endp

; input:
; pNameBuf  fer is an array with at least 49 symbols                                                      
proc getCPUName c pNameBuffer:DWORD
  pusha

  mov edi, [pNameBuffer]
  cld

  mov eax, 80000002h
  
@@:  
  push eax
  cpuid
  stosd                         ; place eax:ebx:ecx:edx symbols to pNameBuffer
  xchg eax, ebx
  stosd
  xchg eax, ecx
  stosd
  xchg eax, edx
  stosd
  pop eax
  inc eax
  cmp eax, 80000004h
  jle @b
  
  xor eax, eax
  stosb
  
  popa
  ret
endp

; input:
; pVenodrBuffer is an array with at least 13 symbols
proc getVendorName c uses eax, pVendorBuffer:DWORD  
  mov edi, [pVendorBuffer]
  cld
  
  xor eax, eax
  cpuid
  
  xchg eax, ebx
  stosd
  xchg eax, edx
  stosd
  xchg eax, ecx
  stosd

  xor eax, eax
  stosb 
  
  ret    
endp


; input:
; pVirtSizeBuffer and pPhysSizeBuffer are the pointer to a dword memory cell
proc getVirtualAndPhysicalAddressSize c pVirtSizeBuffer:DWORD,\
                                        pPhysSizeBuffer:DWORD
  push eax
  push edx
  pushf
  
  mov eax, 80000008h                  ; eax[15;8] - VirtualAddressSize
                                      ; eax[7;0] - PhysicalAddressSize
  push eax
  and eax, 0FFh
  mov edx, [pPhysSizeBuffer]
  mov byte [edx], al
  
  pop eax
  shr eax, 8
  mov edx, [pVirtSizeBuffer]
  mov byte [edx], al
  
  popf
  pop edx
  pop eax
  ret  
endp

proc getPageSize c   
  local sys_info SYSTEM_INFO
  lea eax, [sys_info]  
  invoke GetSystemInfo, eax
  
  xor eax, eax
  mov eax, [sys_info.dwPageSize]
  
  ret
endp

; return value pointers to addresss borders
proc getApplicationAddressBorder c pMinAppAddress:DWORD,\
                                   pMaxAppAddress:DWORD 
  push eax
  push edx                                 
                                    
  local sys_info SYSTEM_INFO
  lea eax, [sys_info]
  invoke GetSystemInfo, eax
  
  mov eax, [sys_info.lpMinimumApplicationAddress]
  mov edx, [pMinAppAddress]
  mov [edx], eax
  
  mov eax, [sys_info.lpMaximumApplicationAddress]
  mov edx, [pMaxAppAddress]
  mov [edx], eax
  
  pop edx
  pop eax
  ret
endp

proc getCPUCount c
  local sys_info SYSTEM_INFO
  lea eax, [sys_info]
  invoke GetSystemInfo, eax
  
  xor eax, eax
  mov eax, [sys_info.dwNumberOfProcessors]
   
  ret
endp

proc getDriveTypeString c pDriveStrName:DWORD
  push ebx
  pushf
  
  mov eax, dword ptr pDriveStrName
  invoke GetDriveTypeA, eax
  
  mov ebx, eax
  
  cmp ebx, 0
  jne @f
  lea eax, [drv_unknown_type]
  jmp .exit
  
@@:  
  cmp ebx, 1
  jne @f
  lea eax, [drv_no_root_type]
  jmp .exit

@@:
  cmp ebx, 2
  jne @f
  lea eax, [drv_rmvble_type]
  jmp .exit
  
@@:  
  cmp ebx, 3
  jne @f
  lea eax, [drv_fixed_type]
  jmp .exit
    
@@:
  cmp ebx, 4
  jne @f
  lea eax, [drv_remote_type]
  jmp .exit    

@@:      
  lea eax, [drv_ram_type]

.exit:
  popf  
  pop ebx
  ret                   
  
endp

proc getFreeDriveSpace pDriveStrName:DWORD, pSpaceBuffer:DWORD
  push eax
  push ebx
  push edx
  pushf

  locals
    sector_per_cluster  dd 0
    bytes_per_sector    dd 0
    free_number_cluster dd 0
    totl_number_cluster dd 0
  endl
  
  mov eax, dword ptr pDriveStrName
  invoke GetDiskFreeSpaceA, eax,\
                            addr sector_per_cluster,\
                            addr bytes_per_sector,\
                            addr free_number_cluster,\
                            addr totl_number_cluster
  
  mov eax, dword ptr free_number_cluster
  mul dword ptr sector_per_cluster
  mul dword ptr bytes_per_sector
  
  mov ebx, [pSpaceBuffer]
  mov [ebx], eax
  mov [ebx+4], edx                          
  
  popf
  pop edx
  pop ebx
  pop eax
  ret
endp

proc getTotalDriveSpace c pDriveStrName:DWORD, pSpaceBuffer:DWORD
  push eax
  push ebx
  push edx
  pushf

  locals
    sector_per_cluster  dd 0
    bytes_per_sector    dd 0
    free_number_cluster dd 0
    totl_number_cluster dd 0
  endl
  
  mov eax, dword ptr pDriveStrName
  invoke GetDiskFreeSpaceA, eax,\
                            addr sector_per_cluster,\
                            addr bytes_per_sector,\
                            addr free_number_cluster,\
                            addr totl_number_cluster
  
  mov eax, dword ptr totl_number_cluster
  mul dword ptr sector_per_cluster
  mul dword ptr bytes_per_sector
  
  mov ebx, [pSpaceBuffer]
  mov [ebx], eax
  mov [ebx+4], edx                          
  
  popf
  pop edx
  pop ebx
  pop eax
  ret
endp

proc getTotalRAMSpace c
  local mem_status MEMORYSTATUS
  invoke GlobalMemoryStatus, addr mem_status
  
  mov eax, dword ptr mem_status.dwTotalPhys

  ret
endp

section '.data' data readable
  drv_unknown_type  db 'UNKNOWN TYPE', 0 
  drv_no_root_type  db 'INVALID DRIVE PATH', 0
  drv_rmvble_type   db 'REMOVABLE DRIVE', 0
  drv_fixed_type    db 'FIXED DRIVE', 0
  drv_remote_type   db 'REMOVE DRIVE', 0
  drv_cdrom_type    db 'CDROM DRIVE', 0
  drv_ram_type      db 'RAM DRIVE', 0

section '.idata' import data readable writeable

  library kernel, 'KERNEL32.DLL'
  
  import kernel,\
    GlobalMemoryStatus, 'GlobalMemoryStatus',\
    GetSystemInfo, 'GetSystemInfo',\
    GetDriveTypeA, 'GetDriveTypeA',\
    GetDiskFreeSpaceA, 'GetDiskFreeSpaceA'

section '.edata' export data readable

  export 'LIBLAB8.DLL',\
    getVendorName, 'getVendorName',\
    getCPUName, 'getCPUName',\
    getVirtualAndPhysicalAddressSize, 'getVirtualAndPhysicalAddressSize', \
    getPageSize, 'getPageSize',\
    getApplicationAddressBorder, 'getApplicationAddressBorder',\
    getCPUCount, 'getCPUCount',\
    getDriveTypeString, 'getDriveTypeString',\
    getFreeDriveSpace, 'getFreeDriveSpace',\
    getTotalDriveSpace, 'getTotalDriveSpace',\
    getTotalRAMSpace, 'getTotalRAMSpace'
    
section '.reloc' fixups data readable discardable
  if $=$$
    dd 0, 8
  end if
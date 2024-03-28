@echo off

set PYTHON_HOME=%PYTHON_HOME_27%

call edksetup.bat

rem build -a IA32
build -a X64

rem efirom -o "Build\MdeModule\DEBUG_VS2019\IA32\SoftGpuGOP.rom" -e "Build\MdeModule\DEBUG_VS2019\IA32\SoftGpuGOP.efi" -l 0x03001 -r 0x01 -f 0xFFFD -i 0x0001
rem efirom -o "Build\MdeModule\DEBUG_VS2019\X64\SoftGpuGOP.rom" -e "Build\MdeModule\DEBUG_VS2019\X64\SoftGpuGOP.efi" -l 0x03001 -r 0x01 -f 0xFFFD -i 0x0001

rem efirom -o "Build\SoftGpu\DEBUG_VS2019\IA32\SoftGpuGopDxe.rom" -e "Build\SoftGpu\DEBUG_VS2019\IA32\SoftGpuGopDxe.efi" -l 0x03001 -r 0x01 -f 0xFFFD -i 0x0001
efirom -o "Build\SoftGpu\DEBUG_VS2019\X64\SoftGpuGopDxe.rom"  -e "Build\SoftGpu\DEBUG_VS2019\X64\SoftGpuGopDxe.efi" -l 0x03001 -r 0x01 -f 0xFFFD -i 0x0001

rem copy "Build\SoftGpuPkg\DEBUG_VS2019\IA32\SoftGpuGOP.rom" "SoftGpuGOP.rom"

copy "Build\SoftGpu\DEBUG_VS2019\X64\SoftGpuGopDxe.rom" "SoftGpuGopDxe.rom"

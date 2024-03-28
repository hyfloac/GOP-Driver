@echo off

set PYTHON_HOME=%PYTHON_HOME_27%

call hysetup.bat

efirom -o "Build\MdeModule\DEBUG_VS2019\IA32\SoftGpuGOP.rom" -e "Build\MdeModule\DEBUG_VS2019\IA32\SoftGpuGOP.efi" -l 0x03001 -r 0x01 -f 0xFFFD -i 0x0001
efirom -o "Build\MdeModule\DEBUG_VS2019\X64\SoftGpuGOP.rom" -e "Build\MdeModule\DEBUG_VS2019\X64\SoftGpuGOP.efi" -l 0x03001 -r 0x01 -f 0xFFFD -i 0x0001

copy "Build\MdeModule\DEBUG_VS2019\X64\SoftGpuGOP.rom" "SoftGpuGOP.rom"
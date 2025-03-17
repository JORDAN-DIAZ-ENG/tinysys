@echo on

cd D:\GitHub\tinysys\software\samples\jordandemoscene\src\

if exist "D:\GitHub\tinysys\software\samples\jordandemoscene\jordandemoscene.elf" (
    del "D:\GitHub\tinysys\software\samples\jordandemoscene\jordandemoscene.elf"
)

make

copy /Y "D:\GitHub\tinysys\software\samples\jordandemoscene\src\jordandemoscene.elf" "D:\GitHub\tinysys\software\emulator\sdcard\jordan"

echo demoscene compiled and placed in jordan folder in sd card

pause
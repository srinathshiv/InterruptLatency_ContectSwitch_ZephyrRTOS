#cp main.c /home/ninja07/temp/backup_zephyr
#cp CMakeLists.txt /home/ninja07/temp/backup_zephyr
#cp execute.sh /home/ninja07/temp/backup_zephyr
#cp board.h /home/ninja07/temp/backup_zephyr 
rm -rf build
mkdir build
cd build
cmake -GNinja -DBOARD=galileo ..
ninja
#expect /home/ninja07/temp/zephyr_kern/zephyr/samples/measure_n/pswd.exp
cd ..


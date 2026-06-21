Here is PTPtool to modify the ptp POSIX clock

to get PTPtool independently run the following commands
```
mkdir PTPtool
cd PTPtool
wget https://raw.githubusercontent.com/Time-Appliances-Project/Incubation-Projects/tree/master/Software/PTPtool/Makefile
wget https://raw.githubusercontent.com/Time-Appliances-Project/Incubation-Projects/tree/master/Software/PTPtool/Softwptptool.cpp
```
Use the following command for compiling:
```
make
```
or use the full command
```
gcc -Wall -lrt ptptool.c -o ptptool
```
optionally, to install
```
sudo mv ptptool /usr/bin/
```

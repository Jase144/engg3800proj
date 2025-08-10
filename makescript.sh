# make script

#!/bin/bash
make clean -C ./Debug

PATH=/home/jesse/st/stm32cubeide_1.19.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.linux64_1.0.0.202410170706/tools/bin:/home/jesse/st/stm32cubeide_1.19.0/plugins/com.st.stm32cube.ide.mcu.externaltools.make.linux64_2.2.0.202409170845/tools/bin:$PATH
make all -C ./Debug

./flash.sh
sleep 1
./flash.sh
./flash.sh
./flash.sh

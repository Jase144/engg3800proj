# Flashing command option params
DEVICE=STM32L433CC
TI=SWD
TRUE=1
FALSE=0
COMMANDFILEPATH=/home/jesse/university/ENGG3800/mk/JLinkCommandFile.jlink
SPEED=100 #4000

# Flashing command options
DBG_FLASH_CLI=JLinkGDBServer
NRM_FLASH_CLI=JLinkExe
DBG_FLASH_OPTIONS="-device $DEVICE -if $TI -speed $SPEED -singlerun -select USB -port 2331"
NRM_FLASH_OPTIONS="-device $DEVICE -If $TI -autoconnect $TRUE -CommandFile $COMMANDFILEPATH -speed 400"
EXTRA_OPTIONS=""

extraOptions=$FALSE
for param in $@
do
    if [[ "$param" == "--" ]]; then
        extraOptions=$TRUE; # set flag to tell the program to expect extra options
    elif [[ $extraOptions == $TRUE ]]; then
       EXTRA_OPTIONS+="$param"
    fi
done

echo # create a new line

# color values
RED='\033[0;31m'
YELLOW='\033[33m'
NC='\033[0m' # No Color

if [[ "$1" == "-d" ]]; then

    echo -e ${RED}DEBUG MODE${NC}
    sudo $DBG_FLASH_CLI $DBG_FLASH_OPTIONS $EXTRA_OPTIONS

    echo  # create a new line to separate output of command and what command was actually sent
    echo
    echo "Command Inputted: $DBG_FLASH_CLI $DBG_FLASH_OPTIONS $EXTRA_OPTIONS"

else # Normal flash

    echo -e ${YELLOW}NORMAL MODE${NC}
    sudo $NRM_FLASH_CLI $NRM_FLASH_OPTIONS $EXTRA_OPTIONS
    echo "Command Inputted: $NRM_FLASH_CLI $NRM_FLASH_OPTIONS $EXTRA_OPTIONS"
fi

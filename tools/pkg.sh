#!/bin/sh
#Author cinience

echo "      ****     "
echo "    *^^^^^^*   "
echo "    * @  @ *   "
echo "   **********  "
echo "  ************ "
echo "   **********  "
echo "   **      **  "
echo "    SMARTPKG   "

CWD=`pwd`
PKGSHELL='pkg.sh'
PKGDIR='package'
INSTLLSHELL='install.sh'
UPGRADESHELL='upgrade.sh'
UNINTSLLSHELL='uninstall.sh'



NORMAL=$(tput sgr0)
GREEN=$(tput setaf 2; tput bold)
YELLOW=$(tput setaf 3)
RED=$(tput setaf 1)


ARCHIVE=`awk '/^__ARCHIVE__/ {print NR; exit 0; }' $0`

LogInfo()
{
  echo -e "[INFO] ${GREEN}$*${NORMAL}"
}

WarnInfo()
{
  echo -e "[WARN] ${YELLOW}$*${NORMAL}"
}

ErrorInfo()
{
  echo -e "[ERROR] ${RED}$*${NORMAL}"
}

CheckDisk()
{
  local pUsed=`df $1 | tail -n1 | awk '{print $5}' | tr -d "%"`
  if [ $pUsed -gt 99 ];then
    ErrorInfo "Filesystem [$1] used ${pUsed}%"
    exit 1
  fi
  return 0
}

Usage()
{
	  ScriptName=`basename $0`
    echo "usage:"
    echo -e "  ${GREEN}sh ${ScriptName} <command>${NORMAL}"
    echo "  -c create    install   package"
    echo "  -x extract   install   package"
    echo "  -i install   install   package"
    echo "  -u upgrade   installed package"
    echo "  -d uninstall installed package"
    echo ""
}

case "$1" in
  -c|--create|-x|--extract)
   export TMPDIR=`pwd`
  ;;
  *)
   export TMPDIR=`mktemp -d /tmp/pkg.XXXXXXX`
  ;;
esac

CheckDisk ${TMPDIR}

case "$1" in 
  -h|--help)
		Usage
  ;;
  -c|--create)
    LogInfo "create install package"
    BinName=$2
    if [ X"${BinName}" = "X" ];then
      BinName='PKG.bin'
    fi
    if [ ! -d ${PKGDIR} ];then
      ErrorInfo "can not find ${PKGDIR}"
      exit 1
    fi
    tar zcf package.tar.gz ${PKGDIR}/
    chmod a+x $0
    dos2unix -q $0
    head -n ${ARCHIVE} $0 > ${BinName}
    cat package.tar.gz >> ${BinName} 
    chmod a+x ${BinName}
    rm -f package.tar.gz
  ;;
  -x|--extract)
    LogInfo "extract install package" 
    head -n ${ARCHIVE} $0 > ${PKGSHELL}
    chmod a+x ${PKGSHELL}
    NPOS=`expr ${ARCHIVE} + 1`
    tail -n+${NPOS} $0 | tar xz -C ${TMPDIR}
  ;;
  -d|--uninstall)
    LogInfo "uninstall installed package"
    NPOS=`expr ${ARCHIVE} + 1`
    tail -n+${NPOS} $0 | tar xz -C ${TMPDIR}
    cd ${TMPDIR}
    find . -name "*.sh" | xargs dos2unix -q
    find . -name "*.sh" | xargs chmod +x 
    cd ${PKGDIR} ; ./${UNINSTLLSHELL}
    cd ${CWD}
    rm -rf ${TMPDIR}
  ;;
  -u|--upgrade)
    LogInfo "upgrade installed package"
    tail -n+${ARCHIVE} $0 | tar xz -C ${TMPDIR}
    cd ${TMPDIR}
    find . -name "*.sh" | xargs dos2unix -q
    find . -name "*.sh" | xargs chmod +x 
    cd ${PKGDIR} ; ./${UPGRADESHELL}
    cd ${CWD}
    rm -rf ${TMPDIR}
  ;;
  *)
    if [ "X$1" = "X-i" ];then
      shift
    fi  
    NPOS=`expr ${ARCHIVE} + 1` 
    Lines=`wc -l $0 | awk '{print $1}'`
    
    tail -n+${NPOS} $0 | tar xz -C ${TMPDIR}
    if [ $? -ne 0 ]  
    then
    	Usage
    	exit 1
    fi
    LogInfo "install package"
    cd ${TMPDIR}
    find . -name "*.sh" | xargs dos2unix -q
    find . -name "*.sh" | xargs chmod +x 
    cd ${PKGDIR} ; ./${INSTLLSHELL} $*
    cd ${CWD}
    rm -rf ${TMPDIR}
  ;;
esac

exit 0

__ARCHIVE__

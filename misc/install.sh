#!/bin/sh

BINPATH=""
CONFFILE=""

USERNAME=`whoami`
CURRENTGID=`id -g`
CMDPREFIX="su root -c"
DESTDIR="/etc/init.d"
CTLFILE="sakerd_${USERNAME}"
TMPFILE="/tmp/${CTLFILE}"


test $# -ge 1 && BINPATH=$1
test $# -ge 2 && CONFFILE=$2

if [ -z ${BINPATH} ];then
    echo "Please input saker path:"
    read BINPATH
fi
echo "USERNAME=${USERNAME}" >  ${TMPFILE}
echo "EXEC=${BINPATH}"      >> ${TMPFILE}
echo "CONF=${CONFFILE}"     >> ${TMPFILE}
cat sakerd.init | grep -v "{TemplateUsername}" | \
                  grep -v "{TemplateExec}" | \
                  grep -v "{TemplateConf}" >> ${TMPFILE}
                  
chmod a+x ${TMPFILE}

test ${CURRENTGID} -eq 0 && mv -f ${TMPFILE} ${DESTDIR}
test ${CURRENTGID} -ne 0 && ${CMDPREFIX} "mv -f ${TMPFILE} ${DESTDIR}/${CTLFILE}"

which chkconfig > /dev/null
if [ $? -ne 0 ] 
then 
    test ${CURRENTGID} -eq 0 && update-rc.d ${CTLFILE} defaults && echo "Success!"
    test ${CURRENTGID} -ne 0 && ${CMDPREFIX} "update-rc.d ${CTLFILE} defaults" && echo "Success!"
else
    test ${CURRENTGID} -eq 0 && chkconfig --add ${CTLFILE} && echo "Successfully added to chkconfig!"
    test ${CURRENTGID} -eq 0 && chkconfig --level 345 ${CTLFILE} on && echo "Successfully added to runlevels 345!"
    
    test ${CURRENTGID} -ne 0 && ${CMDPREFIX} "chkconfig --add ${CTLFILE}" && echo "Successfully added to chkconfig!"
    test ${CURRENTGID} -ne 0 && ${CMDPREFIX} "chkconfig --level 345 ${CTLFILE} on" && echo "Successfully added to runlevels 345!"
fi

${DESTDIR}/${CTLFILE} start



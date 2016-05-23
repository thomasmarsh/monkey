#!/bin/zsh

LOW=40
CHARGED=80

battery() {
    batt=$(pmset -g batt | grep InternalBatter | sed -e 's/%.*//g' | sed -e 's/.*y-0[ ]*//g')
    if [[ -z $batt ]]
    then
        echo 100
    else
        echo $batt
    fi
}

need_charge() {
    (($(battery) < $LOW))
}

charge() {
    while (($(battery) < $CHARGED))
    do
        sleep 60
    done
}

check_charge() {
    if need_charge
    then
        echo 'Wait for battery'
        charge
    fi
}

run_game() {
    log=build/$1.log
    if [ ! -f $log ]
    then
        echo Game $1
        build/monkey > $log
        tail $log
    fi
}

loop() {
    for i in $(seq -w 0 99)
    do
        check_charge
        run_game $i
    done
}

loop

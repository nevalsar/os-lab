IPCS_M=`ipcs -m | egrep "0x[0-9a-f]+ [0-9]+" | grep "666" | cut -f2 -d" "`
for id in $IPCS_M; do                                       
ipcrm -m $id;
done

IPCS_M=`ipcs -s | egrep "0x[0-9a-f]+ [0-9]+" | grep "666" | cut -f2 -d" "`
for id in $IPCS_M; do                                       
ipcrm -s $id;
done

IPCS_M=`ipcs -q | egrep "0x[0-9a-f]+ [0-9]+" | grep "666" | cut -f2 -d" "`
for id in $IPCS_M; do                                       
ipcrm -q $id;
done

rm -rf ser.txt server client commence
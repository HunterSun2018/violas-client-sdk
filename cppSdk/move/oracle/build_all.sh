echo "build oracle.move"
../build.sh oracle.move 0x0
mv move_build_output/modules/0_Oracle.mv oracle.mv

# echo "build modules/vls.move"
# ../build.sh vls.move 0x0
# mv move_build_output/modules/0_VLS.mv vls.mv

# echo "build add_currency_for_designated_dealer.move"
# ../build.sh add_currency_for_designated_dealer.move 0x0
# mv move_build_output/scripts/main.mv add_currency_for_designated_dealer.mv

# echo "build currencies/register_currency.move"
# ../build.sh register_currency.move 0x0
# mv move_build_output/scripts/main.mv register_currency.mv

rm -rf move_build_output
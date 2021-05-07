
#
# build all examples
#    delete any old builds
#    configure and build all projects from their directories
#    gather all binaries (*.hex) in bin directory
#    delete all build direcories
#
#  usage:  ./build_all.sh
#
rm -rf bin
mkdir bin

rm -rf ex_00a_reading_dev_id/build
rm -rf ex_01a_simple_tx/build
rm -rf ex_01b_tx_sleep/build
rm -rf ex_01c_tx_sleep_auto/build
rm -rf ex_01d_tx_timed_sleep/build
rm -rf ex_01e_tx_with_cca/build
rm -rf ex_01g_simple_tx_sts_sdc/build
rm -rf ex_01h_simple_tx_pdoa/build
rm -rf ex_01i_simple_tx_aes/build
rm -rf ex_02a_simple_rx/build
rm -rf ex_02c_rx_diagnostics/build
rm -rf ex_02d_rx_sniff/build
rm -rf ex_02f_rx_with_crystal_trim/build
rm -rf ex_02g_simple_rx_sts_sdc/build
rm -rf ex_02h_simple_rx_pdoa/build
rm -rf ex_02i_simple_rx_aes/build
rm -rf ex_03a_tx_wait_resp/build
rm -rf ex_03b_rx_send_resp/build
rm -rf ex_03d_tx_wait_resp_interrupts/build
rm -rf ex_04a_cont_wave/build
rm -rf ex_04b_cont_frame/build
rm -rf ex_05a_ds_twr_init/build
rm -rf ex_05b_ds_twr_resp/build
rm -rf ex_05c_ds_twr_init_sts_sdc/build
rm -rf ex_05d_ds_twr_resp_sts_sdc/build
rm -rf ex_06a_ss_twr_initiator/build
rm -rf ex_06b_ss_twr_responder/build
rm -rf ex_06e_AES_ss_twr_initiator/build
rm -rf ex_06f_AES_ss_twr_responder/build
rm -rf ex_07a_ack_data_tx/build
rm -rf ex_07b_ack_data_rx/build
rm -rf ex_07c_ack_data_rx_dbl_buff/build
rm -rf ex_11a_spi_crc/build
rm -rf ex_13a_gpio/build
rm -rf ex_14a_otp_write/build
rm -rf ex_15a_le_pend_tx/build
rm -rf ex_15b_le_pend_rx/build

pushd .; cd ex_00a_reading_dev_id           ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01a_simple_tx                ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01b_tx_sleep                 ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01c_tx_sleep_auto            ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01d_tx_timed_sleep           ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01e_tx_with_cca              ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01g_simple_tx_sts_sdc        ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01h_simple_tx_pdoa           ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_01i_simple_tx_aes            ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02a_simple_rx                ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02c_rx_diagnostics           ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02d_rx_sniff                 ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02f_rx_with_crystal_trim     ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02g_simple_rx_sts_sdc        ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02h_simple_rx_pdoa           ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_02i_simple_rx_aes            ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_03a_tx_wait_resp             ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_03b_rx_send_resp             ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_03d_tx_wait_resp_interrupts  ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_04a_cont_wave                ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_04b_cont_frame               ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_05a_ds_twr_init              ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_05b_ds_twr_resp              ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_05c_ds_twr_init_sts_sdc      ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_05d_ds_twr_resp_sts_sdc      ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_06a_ss_twr_initiator         ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_06b_ss_twr_responder         ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_06e_aes_ss_twr_initiator     ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_06f_aes_ss_twr_responder     ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_07a_ack_data_tx              ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_07b_ack_data_rx              ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_07c_ack_data_rx_dbl_buff     ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_11a_spi_crc                  ; ./configure.sh; cd build; make -j4; popd;
pushd .; cd ex_13a_gpio                     ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_14a_otp_write                ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_15a_le_pend_tx               ; ./configure.sh; cd build; make -j4; popd
pushd .; cd ex_15b_le_pend_rx               ; ./configure.sh; cd build; make -j4; popd

cp ./ex_00a_reading_dev_id/build/zephyr/zephyr.hex           ./bin/ex_00a_reading_dev_id.hex
cp ./ex_01a_simple_tx/build/zephyr/zephyr.hex                ./bin/ex_01a_simple_tx.hex
cp ./ex_01b_tx_sleep/build/zephyr/zephyr.hex                 ./bin/ex_01b_tx_sleep.hex
cp ./ex_01c_tx_sleep_auto/build/zephyr/zephyr.hex            ./bin/ex_01c_tx_sleep_auto.hex
cp ./ex_01d_tx_timed_sleep/build/zephyr/zephyr.hex           ./bin/ex_01d_tx_timed_sleep.hex
cp ./ex_01e_tx_with_cca/build/zephyr/zephyr.hex              ./bin/ex_01e_tx_with_cca.hex
cp ./ex_01g_simple_tx_sts_sdc/build/zephyr/zephyr.hex        ./bin/ex_01g_simple_tx_sts_sdc.hex
cp ./ex_01h_simple_tx_pdoa/build/zephyr/zephyr.hex           ./bin/ex_01h_simple_tx_pdoa.hex
cp ./ex_01i_simple_tx_aes/build/zephyr/zephyr.hex            ./bin/ex_01i_simple_tx_aes.hex
cp ./ex_02a_simple_rx/build/zephyr/zephyr.hex                ./bin/ex_02a_simple_rx.hex
cp ./ex_02c_rx_diagnostics/build/zephyr/zephyr.hex           ./bin/ex_02c_rx_diagnostics.hex
cp ./ex_02d_rx_sniff/build/zephyr/zephyr.hex                 ./bin/ex_02d_rx_sniff.hex
cp ./ex_02f_rx_with_crystal_trim/build/zephyr/zephyr.hex     ./bin/ex_02f_rx_with_crystal_trim.hex
cp ./ex_02g_simple_rx_sts_sdc/build/zephyr/zephyr.hex        ./bin/ex_02g_simple_rx_sts_sdc.hex
cp ./ex_02h_simple_rx_pdoa/build/zephyr/zephyr.hex           ./bin/ex_02h_simple_rx_pdoa.hex
cp ./ex_02i_simple_rx_aes/build/zephyr/zephyr.hex            ./bin/ex_02i_simple_rx_aes.hex
cp ./ex_03a_tx_wait_resp/build/zephyr/zephyr.hex             ./bin/ex_03a_tx_wait_resp.hex
cp ./ex_03b_rx_send_resp/build/zephyr/zephyr.hex             ./bin/ex_03b_rx_send_resp.hex
cp ./ex_03d_tx_wait_resp_interrupts/build/zephyr/zephyr.hex  ./bin/ex_03d_tx_wait_resp_interrupts.hex
cp ./ex_04a_cont_wave/build/zephyr/zephyr.hex                ./bin/ex_04a_cont_wave.hex
cp ./ex_04b_cont_frame/build/zephyr/zephyr.hex               ./bin/ex_04b_cont_frame.hex
cp ./ex_05a_ds_twr_init/build/zephyr/zephyr.hex              ./bin/ex_05a_ds_twr_init.hex
cp ./ex_05b_ds_twr_resp/build/zephyr/zephyr.hex              ./bin/ex_05b_ds_twr_resp.hex
cp ./ex_05c_ds_twr_init_sts_sdc/build/zephyr/zephyr.hex      ./bin/ex_05c_ds_twr_init_sts_sdc.hex
cp ./ex_05d_ds_twr_resp_sts_sdc/build/zephyr/zephyr.hex      ./bin/ex_05d_ds_twr_resp_sts_sdc.hex
cp ./ex_06a_ss_twr_initiator/build/zephyr/zephyr.hex         ./bin/ex_06a_ss_twr_initiator.hex
cp ./ex_06b_ss_twr_responder/build/zephyr/zephyr.hex         ./bin/ex_06b_ss_twr_responder.hex
cp ./ex_06e_aes_ss_twr_initiator/build/zephyr/zephyr.hex     ./bin/ex_06e_aes_ss_twr_initiator.hex
cp ./ex_06f_aes_ss_twr_responder/build/zephyr/zephyr.hex     ./bin/ex_06f_aes_ss_twr_responder.hex
cp ./ex_07a_ack_data_tx/build/zephyr/zephyr.hex              ./bin/ex_07a_ack_data_tx.hex
cp ./ex_07b_ack_data_rx/build/zephyr/zephyr.hex              ./bin/ex_07b_ack_data_rx.hex
cp ./ex_07c_ack_data_rx_dbl_buff/build/zephyr/zephyr.hex     ./bin/ex_07c_ack_data_rx_dbl_buff.hex
cp ./ex_11a_spi_crc/build/zephyr/zephyr.hex                  ./bin/ex_11a_spi_crc.hex
cp ./ex_13a_gpio/build/zephyr/zephyr.hex                     ./bin/ex_13a_gpio.hex
cp ./ex_14a_otp_write/build/zephyr/zephyr.hex                ./bin/ex_14a_otp_write.hex
cp ./ex_15a_le_pend_tx/build/zephyr/zephyr.hex               ./bin/ex_15a_le_pend_tx.hex
cp ./ex_15b_le_pend_rx/build/zephyr/zephyr.hex               ./bin/ex_15b_le_pend_rx.hex

rm -rf ex_00a_reading_dev_id/build
rm -rf ex_01a_simple_tx/build
rm -rf ex_01b_tx_sleep/build
rm -rf ex_01c_tx_sleep_auto/build
rm -rf ex_01d_tx_timed_sleep/build
rm -rf ex_01e_tx_with_cca/build
rm -rf ex_01g_simple_tx_sts_sdc/build
rm -rf ex_01h_simple_tx_pdoa/build
rm -rf ex_01i_simple_tx_aes/build
rm -rf ex_02a_simple_rx/build
rm -rf ex_02c_rx_diagnostics/build
rm -rf ex_02d_rx_sniff/build
rm -rf ex_02f_rx_with_crystal_trim/build
rm -rf ex_02g_simple_rx_sts_sdc/build
rm -rf ex_02h_simple_rx_pdoa/build
rm -rf ex_02i_simple_rx_aes/build
rm -rf ex_03a_tx_wait_resp/build
rm -rf ex_03b_rx_send_resp/build
rm -rf ex_03d_tx_wait_resp_interrupts/build
rm -rf ex_04a_cont_wave/build
rm -rf ex_04b_cont_frame/build
rm -rf ex_05a_ds_twr_init/build
rm -rf ex_05b_ds_twr_resp/build
rm -rf ex_05c_ds_twr_init_sts_sdc/build
rm -rf ex_05d_ds_twr_resp_sts_sdc/build
rm -rf ex_06a_ss_twr_initiator/build
rm -rf ex_06b_ss_twr_responder/build
rm -rf ex_06e_aes_ss_twr_initiator/build
rm -rf ex_06f_aes_ss_twr_responder/build
rm -rf ex_07a_ack_data_tx/build
rm -rf ex_07b_ack_data_rx/build
rm -rf ex_07c_ack_data_rx_dbl_buff/build
rm -rf ex_11a_spi_crc/build
rm -rf ex_13a_gpio/build
rm -rf ex_14a_otp_write/build
rm -rf ex_15a_le_pend_tx/build
rm -rf ex_15b_le_pend_rx/build

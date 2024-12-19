- Software Setup
    - On Host
        make      # on parent folder to copy to NFS
    - On Target: 
        config-pin P8.11 pruout   # This is for outputting to LEDs
        config-pin P8.15 pruin    # This is for taking input Right Joystick
        config-pin p8.16 pruin    # This is for taking input Down Joystick
        make
        make install_PRU0
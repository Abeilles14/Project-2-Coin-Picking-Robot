SHELL=cmd
CC=c51
COMPORT = $(shell type COMPORT.inc)
OBJS=coin_picking_arm.obj nrf24.obj radioPinFunctions.obj

tx.hex: $(OBJS)
	$(CC) $(OBJS)
	@echo Done!
	
coin_picking_arm.obj: coin_picking_arm.c
	$(CC) -c coin_picking_arm.c

nrf24.obj: nrf24.c
	$(CC) -c nrf24.c

radioPinFunctions.obj: radioPinFunctions.c
	$(CC) -c radioPinFunctions.c

clean:
	@del *.asm *.lkr *.lst *.map *.hex *.obj 2> nul

LoadFlash:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	EFM8_prog.exe -ft230 -r coin_picking_arm.hex
	cmd /c start c:\PUTTY\putty -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start c:\PUTTY\putty -serial $(COMPORT) -sercfg 115200,8,n,1,N -v

Dummy: coin_picking_arm.hex coin_picking_arm.Map
	@echo Nothing to see here!
	
explorer:
	cmd /c start explorer .
		
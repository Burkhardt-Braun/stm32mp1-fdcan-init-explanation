# stm32mp1-fdcan-init-explanation
These code snippets document an issue while clearing the FDCAN CCCR.INIT bit on STM32MP1 devices. The root cause in this case was an insufficient FDCAN kernel clock frequency (fdcan_ker_ck below 80 MHz). The repository also shows the PLL4_R clock setup and INIT-to-RUN transition sequence.

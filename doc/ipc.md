# Inter-Process Communication

Communication between the processes approximately follows the following flow diagram. Note that the PIO state machines are excluded from the diagram. They are implicitly contained "within" core 1 to allow the cores to use the scan chain.

![Inter-process communication diagram between sequencer and module](img/ipc.png)



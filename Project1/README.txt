Soohyeuk Choi
sc9429@nyu.edu


I have finished the assignment after using bitset library, E20 manual, given test cases, and PTC. 

My plan of design started after the following: 
	- I thoroughly read through the given starting code, from load_machine_code function to the main function.
	- I changed the parameter of those given starting functions based on my need (unsigned --> uint16_t). 
	

After a clear understanding of the starting code and the changes of the parameter, I began on the implementation. 

Firstly, I set multiple variables to represent, PC, Registers, and Memory, where they were all uint16_t, because we want all of them to be a 16-bit value, based on E20 manual. Register and Memory are an array, which holds each register/instruction. Then, I created a function for the simulator and a loop that will only end when a variable "halt" is set to False.

In the loop, I created multiple variables where it abstracts instructions from the memory and parse it down into opCode, imm, etc.. I created all of them in the beginning, so that the code can be concise and organized, since creating these variables will take a very insignificant number of time. I parsed it using shift operators and bitwise operators. 

It starts with a huge if-else statement that will execute different instructions based on the opcode. Each if-else statement will do what was explained on the E20 manual (i.e. adding regSrc B and regSrc A and storing it to regDst and pc++). For a little more explanation, for when opCode is 0, there is a nested if-statement, as different instructions will be executed depending on the 4-imm value. Some details I considered while designing this code were: 

 - making sure that $0 does not get modified by having a small if-statement that checks if the register we try to modify is $0. 
 - making sure that pc increments by 1, naturally unless specified by the instruction (i.e. j). 
 - making sure that the simulator ends when pc increments but it overflows and comes back to the same instruction by making a variable of the original pc and comparing the value of modified pc to that variable. 
 - making sure that each instruction has its own if-statement, so it can have an individual instruction to execute. 

While working on some instructions, I found there to be an issue with arithmetics due to negative numbers (which can be add, but also certain jump instructions and lw/sw). For that reason, I created sign-extend function separately to make sure all the arithmetic can be done properly when given negative value as 7-imm value. It essentially does a bitwise operation (or) when the MSB is 1.

My last touch of edit was on making sure small details were correct like: 
 - Dereferencing parameters properly 
 - C++ syntax issues
 - PC overflow 
 - All instruction behaviors appropriately according to the E20 manual. 
















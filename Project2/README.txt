NAME: Soohyeuk Choi 
sc9429@nyu.edu


I have finished the assignment after using E20 manual, given test cases, help from TA and PTC. 

My plan of design started after the following: 
	- I thoroughly read through the given starting code, from print functions to main function.
	- I imported the previous E20 simulator and fixed all the errors. 
	- I changed the parameters of starter functions if it was necessary. 

After a clear understanding of the starting code and the changes of the parameter, I began on the implementation. 

First, I implemented cache by constructing 4 structs: Cache -> Level -> Row -> Block. Cache contains a vector of Level, Level contains a vector row, ..., and Block contains uint16_t tag set to -1 initially. This design came along after thinking about how each things should be accessed and how efficiently I can access each. 


Then I created a cache_func that passed in multiple parameters including Cache struct. I created this function to do all calculations of row, tag, block size, and hit/miss. It also accounts for when the function should try to attempt to access L2 cache. 
Some things to consider when creating this function was:
1.if the size of vector in a Cache struct is 2
2.if the instruction is SW or if it's a miss 
3. One and Two has to be true to search through the L2 cache

Initially, I had all calculations in one function, cache_func, but it was getting too complicated and big, so I separated into two: cache_helper, and cache_func. Cache_helper does LRU policy and if it's a miss/hit. Cache_func calculates tag and more information and check if it should access L2 cache based on given information by cache_helper and its existing information. 

After creating these functions and cache, I started to add print statements in these two, so I can follow the project instructions. I used the given starter functions to print. Then, I modified E20 simulator's parameters so it can accept cache and operate cache_func(). 

Then I also modified main func in order to create a main cache, and to use print functions. 

Some issues I had were:
 - Struct issues
 - Memory issues
 - design issues 




















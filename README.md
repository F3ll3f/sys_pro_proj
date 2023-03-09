# sys_pro_proj

Eleftherios Bolkas

This is a project made for a System Programming course in the Department of Informatics and Telecommunications in the National And Kapodistrian University of Athens.

This project consists of 3 parts. Each part includes in its folder a "README.txt" that provides a detailed explanation of the respective part of the project and of its implementation. Brief description of each part: 

Part1: In this part of the project, a vaccine application is developed. Vaccination data for a population can be inserted to the application and the application can provide various vaccination stats about a specific person or the whole population. For the development of the application, structures such as BloomFilter and SkipList have been implemented. A bash script that provides test data has also been implemented to test the application.

Part2: In this part, the vaccine application of part1 is updated so that it can be used for traveling purposes. Specifically it accepts requests from citizens that want to travel to a country and informs them if their vaccination status allows them to travel. For its efficient use, multiple proccess(monitors) are created that handle the requests sent from a parent process. The proccesses communicate with the parent process through named pipes and signals. A bash script that provides test data has also been implemented to test the updated application too.

Part3: In this part, the vaccine application of part2 is redisigned in order to become multithreaded. The commumnicatiton between the parent thread and the child threads(monitors) happens through sockets. The monitors use a common buffer for some of their operations and mutexes are used for its safe use.



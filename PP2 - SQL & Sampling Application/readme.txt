
CS 564: Database Management Systems
PP 2: SQL
Feb. 10, 2020
How to run
* Use make sql to run 10 SQL queries for part A
* Use make run-jar to run the executable jar file for part B
Assumptions
* The JDBC application connects to the course database cs564instr on Linux lab at UW-Madison. 
* It reads from hw2 schema and all tables are created under the public schema. 
* It is assumed that the user has the permission to those schemas under cs564instr database. 
* The file login.config is present in the same folder as executable.jar


Files
* Readme.txt: Readme file in txt format
* Readme.pdf: Readme file in pdf format
* query1.sql - query10.sql: one file per question, including the resulted rows in the comment
* pgtest.java: Java code that shows the prompt and output data based on input information
* executable.jar: Compiled jar file for pgtest.java. Use make run-jar to run the file

Output

[szhong@snares-03] (1)$ make
javac pgtest.java
java -Djava.security.auth.login.config=login.config -classpath ./postgresql-8.4-703.jdbc4.jar:. pgtest
Debug is  true storeKey false useTicketCache true useKeyTab false doNotPrompt false ticketCache is null isInitiator true KeyTab is null refreshKrb5Config is false principal is null tryFirstPass is false useFirstPass is false storePass is false clearPass is false
Acquire TGT from Cache
Principal is szhong@CS.WISC.EDU
Commit Succeeded 

Please specify execution mode (Enter T (Table) / Q (Query) / E (Exit)): t
Please enter table name: holidays
How many samples do you want: 10
Do you want to set seed? (Enter Y/N): n
Use previous seed: 0
Please select output mode (Enter S (Save in new table) / O (Output)): o
weekdate        isholiday        
2010-05-07        false        
2010-10-08        false        
2010-10-15        false        
2010-12-24        false        
2011-02-11        true        
2011-06-17        false        
2011-07-01        false        
2011-09-09        true        
2011-10-07        false        
2012-02-03        false        
Do you want to continue sampling? (Enter Y/N): n

=============================================================================

Please specify execution mode (Enter T (Table) / Q (Query) / E (Exit)): t
Please enter table name: stores
How many samples do you want: 10  5
Do you want to set seed? (Enter Y/N): y
Please enter the seed for sampling: 39
Please select output mode (Enter S (Save in new table) / O (Output)): o
store        type        size        
33        A        39690        
39        A        184109        
40        A        155083        
41        A        196321        
43        C        41062        
Do you want to continue sampling? (Enter Y/N): y
How many samples do you want: 5
Do you want to set seed? (Enter Y/N): y  
Please enter the seed for sampling: 20
Please select output mode (Enter S (Save in new table) / O (Output)): o
store        type        size        
9        B        125833        
26        A        152513        
27        A        204184        
32        A        203007        
38        C        39690        
Do you want to continue sampling? (Enter Y/N): n

=============================================================================

Please specify execution mode (Enter T (Table) / Q (Query) / E (Exit)): temp   
Please enter table name: temporaldata
How many samples do you want: 10
Do you want to set seed? (Enter Y/N): n
Use previous seed: 20
Please select output mode (Enter S (Save in new table) / O (Output)): o
store        weekdate        temperature        fuelprice        cpi        unemploymentrate        
1        2011-03-11        53.56        3.459        214.11105        7.742        
1        2012-01-27        54.26        3.29        220.07886        7.348        
3        2010-06-18        83.52        2.637        214.78583        7.343        
19        2010-11-12        40.3        3.065        132.97832        8.067        
24        2011-08-12        72.52        3.995        136.14413        8.358        
32        2012-01-20        36.86        3.055        196.77966        8.256        
35        2011-02-25        33.05        3.274        137.37645        8.549        
35        2011-07-01        73.76        3.748        139.36375        8.684        
35        2012-10-12        55.4        4.0        142.93762        8.665        
44        2011-08-12        75.95        3.606        129.20158        6.56        
Do you want to continue sampling? (Enter Y/N): n

=============================================================================

Please specify execution mode (Enter T (Table) / Q (Query) / E (Exit)): t
Please enter table name: sales
How many samples do you want: 10
Do you want to set seed? (Enter Y/N): y
Please enter the seed for sampling: 0
Please select output mode (Enter S (Save in new table) / O (Output)): o
store        dept        weekdate        weeklysales        
7        87        2011-03-04        11488.98        
8        59        2012-09-07        203.28        
13        17        2011-09-30        15799.2        
18        95        2011-03-25        49958.1        
21        42        2012-01-06        5561.71        
29        6        2010-07-09        4793.38        
37        59        2011-09-09        66.07        
40        36        2012-06-08        1300.0        
43        4        2011-05-13        18788.87        
44        10        2011-11-18        192.1        
Do you want to continue sampling? (Enter Y/N): n

=============================================================================

Please specify execution mode (Enter T (Table) / Q (Query) / E (Exit)): e
[szhong@snares-03] (2)$


Javadoc: (see the pdf file)

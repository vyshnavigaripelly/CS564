all: run-java

compile-java:
	javac pgtest.java

run-java: compile-java
	java -Djava.security.auth.login.config=login.config -classpath ./postgresql-8.4-703.jdbc4.jar:. pgtest

compile-jar: compile-java
	jar cfm executable.jar MANIFEST.MF *.class login.config postgresql-8.4-703.jdbc4.jar

run-jar: compile-jar
	java -Djava.security.auth.login.config=login.config -jar executable.jar

clean:
	rm -rf *.class
	rm -rf executable.jar

sql:
	psql < query1.sql
	psql < query2.sql
	psql < query3.sql
	psql < query4.sql
	psql < query5.sql
	psql < query6.sql
	psql < query7.sql
	psql < query8.sql
	psql < query9.sql
	psql < query10.sql


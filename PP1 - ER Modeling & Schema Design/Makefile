main:
	sh runParser.sh
	sqlite3 ebay_data.db < create.sql
	sqlite3 ebay_data.db < load.txt

test: 
	sqlite3 ebay_data.db < query1.sql
	sqlite3 ebay_data.db < query2.sql
	sqlite3 ebay_data.db < query3.sql
	sqlite3 ebay_data.db < query4.sql
	sqlite3 ebay_data.db < query5.sql
	sqlite3 ebay_data.db < query6.sql
	sqlite3 ebay_data.db < query7.sql
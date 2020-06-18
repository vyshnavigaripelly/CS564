init:
	cd create_auctionbase && sh runParser.sh && sh createDatabase.sh && cp AuctionBase ../web.py

run:
	cd web.py && python auctionbase.py

clean:
	rm -f create_auctionbase/*.dat
	rm -f create_auctionbase/AuctionBase
	rm -f web.py/AuctionBase
	rm -f web.py/*.pyc

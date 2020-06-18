DROP TABLE if exists Category;
DROP TABLE if exists Country;
DROP TABLE if exists Location;
DROP TABLE if exists User;
DROP TABLE if exists Item;
DROP TABLE if exists ItemCategory;
DROP TABLE if exists ItemSeller;
DROP TABLE if exists Bid;
DROP TABLE if exists ItemBid;

-- This is a category table including all categories presented in the 
-- given data. A category_id is associated with each category and is 
-- used to link each item with its categories in ItemCategory table.
CREATE TABLE Category
(
 category_id   INT          NOT NULL UNIQUE,
 category_name VARCHAR(255) NOT NULL UNIQUE,
 PRIMARY KEY (category_id)
);
 
-- This is a country table including all countries presented in the 
-- given data. A country_id is associated with each country and is 
-- used in the location table to link each location with its country
CREATE TABLE Country 
(
 country_id   INT          NOT NULL UNIQUE,
 country_name VARCHAR(255) NOT NULL UNIQUE,
 PRIMARY KEY (country_id)
);
 
-- This is a location table including all locations presented in the 
-- given data. A location id is associated with each location as an 
-- unique not null primary key. This table also links each location 
-- with its corresponding country’s id as a foreign key named 
-- courtry_id referencing the country_id in the Country table.
CREATE TABLE Location
(
 location_id INT          NOT NULL UNIQUE,
 location    VARCHAR(255) NOT NULL UNIQUE,
 country_id  INT,
 PRIMARY KEY (location_id),
 FOREIGN KEY (country_id) REFERENCES Country (country_id)
);
-- This is a user table including all buyers and bidders present in
-- the given data. A location_id is associated with each location in 
-- the Location table 
CREATE TABLE User
(
 user_id     VARCHAR(255) NOT NULL UNIQUE,
 rating      INT          NOT NULL,
 location_id INT,
 PRIMARY KEY (user_id),
 FOREIGN KEY (location_id) REFERENCES Location (location_id)
);
 
-- This is an item table including all items presented in the given 
-- data. It records the name, current price, buy price, first bid, 
-- number of bids, location, start and end dates, and the description 
-- of the item. Each item also has a unique identifier item_id that 
-- associate it with its bids, its sellers, and other information. 
CREATE TABLE Item
(
 item_id        INT          NOT NULL UNIQUE,
 name           VARCHAR(255) NOT NULL,
 currently      DOUBLE,
 buy_price      DOUBLE,
 first_bid      DOUBLE,
 number_of_bids INT          NOT NULL,
 location_id    INT          NOT NULL,
 started        datetime     NOT NULL,
 ends           datetime     NOT NULL,
 description    VARCHAR(255) NOT NULL,
 PRIMARY KEY (item_id),
 FOREIGN KEY (first_bid) REFERENCES Bid (bid_id),
 FOREIGN KEY (location_id) REFERENCES Location (location_id)
);
 

-- This is table to store the relation between each item and its 
-- corresponding category. Neither item_id nor category_id is unique, 
-- but their pairs are unique and used as primary key.
CREATE TABLE ItemCategory
(
 item_id     INT NOT NULL,
 category_id INT NOT NULL,
 PRIMARY KEY (item_id, category_id),
 FOREIGN KEY (item_id) REFERENCES Item (item_id),
 FOREIGN KEY (category_id) REFERENCES Category (category_id)
);


-- The ItemSeller table is a relation table that records the 
-- relations between Items which are represented by their item_id and 
-- Sellers which are users that represented by seller_id as a foreign 
-- key referencing the user_id in the User table. The primary key is 
-- established to be a super key of the pair (item_id, seller_id) 
-- which means both item_id and seller_id are not the primary keys 
-- independently but the pair forms a primary key 
CREATE TABLE ItemSeller
(
 item_id   INT NOT NULL UNIQUE,
 seller_id VARCHAR(255) NOT NULL,
 PRIMARY KEY (item_id, seller_id),
 FOREIGN KEY (seller_id) REFERENCES USER (user_id)
);
 
-- This is a bid table including all bids presented in the given 
-- data. Any bid is uniquely identifiable by the bid_id. The 
-- bidder_id is a foreign key referencing the user_id from the User 
-- table. Each bid also has a bid_time and an amount. 
CREATE TABLE Bid
(
 bid_id    INT      NOT NULL UNIQUE,
 bidder_id VARCHAR(255)      NOT NULL,
 bid_time  datetime NOT NULL,
 amount    DOUBLE   NOT NULL,
 PRIMARY KEY (bid_id),
 FOREIGN KEY (bidder_id) REFERENCES USER (user_id)
);
 

-- This table associates each bid with the identifier of its 
-- corresponding item. The identifier of each bid serves as the key to 
-- this table since each bid can only associate with one item. 
CREATE TABLE ItemBid
(
 item_id INT NOT NULL,
 bid_id  INT NOT NULL UNIQUE,
 PRIMARY KEY (bid_id),
 FOREIGN KEY (bid_id) REFERENCES Bid (bid_id),
 FOREIGN KEY (item_id) REFERENCES Item (item_id)
);


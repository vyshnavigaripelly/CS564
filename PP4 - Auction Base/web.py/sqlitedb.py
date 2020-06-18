import web

db = web.database(dbn='sqlite',
        db='AuctionBase'
    )

######################BEGIN HELPER METHODS######################

# Enforce foreign key constraints
# WARNING: DO NOT REMOVE THIS!
def enforceForeignKey():
    db.query('PRAGMA foreign_keys = ON')

# initiates a transaction on the database
def transaction():
    return db.transaction()
# Sample usage (in auctionbase.py):
#
# t = sqlitedb.transaction()
# try:
#     sqlitedb.query('[FIRST QUERY STATEMENT]')
#     sqlitedb.query('[SECOND QUERY STATEMENT]')
# except Exception as e:
#     t.rollback()
#     print str(e)
# else:
#     t.commit()
#
# check out http://webpy.org/cookbook/transactions for examples

# returns the current time from your database
def getTime():
    # the correct column and table name in your database
    query_string = 'select Time from CurrentTime'
    results = query(query_string)
    return results[0].Time 

# returns a single item specified by the Item's ID in the database
# Note: if the `result' list is empty (i.e. there are no items for a
# a given ID), this will throw an Exception!
def getItemById(item_id):
    query_string = 'select * from Items where item_ID = $itemID'
    result = query(query_string, {'itemID': item_id})
    try:
        return result[0]
    except Exception as e:
        return []

# wrapper method around web.py's db.query method
# check out http://webpy.org/cookbook/query for more info
def query(query_string, vars = {}):
    return list(db.query(query_string, vars))

#####################END HELPER METHODS#####################

# e.g. to update the current time

def searchItem(item_id, category='', description='', user_id='', min_price='', max_price='', status=''):
    from_table = 'Items '
    where_cond = ' '

    if category != '':
        from_table += ', Categories '
        where_cond += 'Categories.category = $category and Categories.ItemID = Items.ItemID and '
        
    if item_id != '':
        where_cond += 'Items.ItemID = $item_id and '
    
    if description != '':
        description = '%' + description + '%' 
        where_cond += 'Items.Description like $description and '
    
    if user_id != '':
        where_cond += 'Items.Seller_UserID = $user_id and '
    
    if min_price != '':
        where_cond += 'Items.Currently >= $min_price and '
    
    if max_price != '':
        where_cond += 'Items.Currently <= $max_price and '
    
    if status == 'open':
        where_cond += 'Items.Started <= $time and Items.Ends >= $time and '
    elif status == 'close':
        where_cond += '(Items.Ends <= $time or Items.Currently >= Items.Buy_Price) and '
    elif status == 'notStarted':
        where_cond += 'Items.Started >= $time and '

    column = ['ItemID', 'Name', 'Currently', 'First_Bid', 'Buy_Price', 'Number_of_Bids', 'Started', 'Ends', 'Seller_UserID', 'Description']
    column = ['Items.' + x for x in column]
    column = ", ".join(column)
    query_string = 'select ' + column + ' from ' + from_table + 'where' + where_cond + '1 == 1 LIMIT 10;'

    vars = {
        'item_id': item_id, 
        'user_id' : user_id,
        'max_price': max_price,
        'min_price': min_price,
        'category': category,
        'description': description,
        'time': getTime()
    }
    result = query(query_string, vars)
    return result

def addBid(item_id, user_id, price):
    # start error handling
    # check valid user
    result = query('select * from Users where UserId == $user_id', {'user_id': user_id})
    if len(result) == 0: 
        return 'Invalid user id'

    # check Seller_UserID != user_id and valid item_id
    result = query('select * from Items where ItemId == $item_id and Seller_UserId <> $user_id', {'item_id': item_id, 'user_id': user_id})
    if len(result) == 0 :
        return 'No biddable item'
    
    item = result[0]

    # check start < current_time < end
    current_time = getTime()
    if item['Started'] > current_time or item['Ends'] < current_time:
        return 'Bid closed due to time'
    
    # check buy_price > currently 
    if item["Buy_Price"] != None and float(item["Buy_Price"]) <= float(item["Currently"]):
        return 'Bid closed since price reached'
    
    # check price > currently
    if float(price) <= float(item["Currently"]): 
        return 'Not higher bid'

    # end error handling

    # start transaction
    t = transaction()
    try:
        # Add to Bids
        insert_bid = "insert into Bids values ($item_id, $user_id, $price, $current_time)"
        db.query(insert_bid, {'item_id': item_id, 'user_id': user_id, 'price': price, 'current_time': current_time})
    except Exception as e:
        t.rollback()
        return str(e) 
    else:
        t.commit()
    # end transaction

    return 'Success'

def setTime(time): 
    query_string = 'update CurrentTime set Time = $currTime;'
    # query_string = 'select * from CurrentTime'
    db.query(query_string, {'currTime': time})
    
def getBids(item_id): 
    query_string = 'select UserID, Amount, Time from Bids where ItemID = $item_id order by Time'
    result = query(query_string, {'item_id': item_id})
    return result

def getCategory(item_id): 
    query_string = 'select Category from Categories where ItemID = $item_id'
    result = query(query_string, {'item_id': item_id})
    return [elem['Category'] for elem in result]

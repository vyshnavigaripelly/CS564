#!/usr/bin/env python

import sys; sys.path.insert(0, 'lib') # this line is necessary for the rest
import os                             # of the imports to work!

import web
import sqlitedb
from jinja2 import Environment, FileSystemLoader
from datetime import datetime

###########################################################################################
##########################DO NOT CHANGE ANYTHING ABOVE THIS LINE!##########################
###########################################################################################

######################BEGIN HELPER METHODS######################

# helper method to convert times from database (which will return a string)
# into datetime objects. This will allow you to compare times correctly (using
# ==, !=, <, >, etc.) instead of lexicographically as strings.

# Sample use:
# current_time = string_to_time(sqlitedb.getTime())

def string_to_time(date_str):
    return datetime.strptime(date_str, '%Y-%m-%d %H:%M:%S')

# helper method to render a template in the templates/ directory
#
# `template_name': name of template file to render
#
# `**context': a dictionary of variable names mapped to values
# that is passed to Jinja2's templating engine
#
# See curr_time's `GET' method for sample usage
#
# WARNING: DO NOT CHANGE THIS METHOD
def render_template(template_name, **context):
    extensions = context.pop('extensions', [])
    globals = context.pop('globals', {})

    jinja_env = Environment(autoescape=True,
            loader=FileSystemLoader(os.path.join(os.path.dirname(__file__), 'templates')),
            extensions=extensions,
            )
    jinja_env.globals.update(globals)

    web.header('Content-Type','text/html; charset=utf-8', unique=True)

    return jinja_env.get_template(template_name).render(context)

#####################END HELPER METHODS#####################

urls = ('/currtime', 'curr_time',
        '/selecttime', 'select_time',
        '/addbid', 'add_bid',
        '/search', 'search',
        '/timetable', 'time_table',
        '/', 'search',
        '/item', 'item'
        )

class curr_time:
    # A simple GET request, to '/currtime'
    #
    # Notice that we pass in `current_time' to our `render_template' call
    # in order to have its value displayed on the web page
    def GET(self):
        current_time = sqlitedb.getTime()
        return render_template('curr_time.html', time = current_time)

class select_time:
    def GET(self):
        return render_template('select_time.html')

    # A POST request
    #
    # You can fetch the parameters passed to the URL
    # by calling `web.input()' for **both** POST requests
    # and GET requests
    def POST(self):
        post_params = web.input()
        MM = post_params['MM']
        dd = post_params['dd']
        yyyy = post_params['yyyy']
        HH = post_params['HH']
        mm = post_params['mm']
        ss = post_params['ss']
        enter_name = post_params['entername']


        selected_time = '%s-%s-%s %s:%s:%s' % (yyyy, MM, dd, HH, mm, ss)
        update_message = '(Hello, %s. Previously selected time was: %s.)' % (enter_name, selected_time)
        try:
            sqlitedb.setTime(string_to_time(selected_time))
        except Exception as e:
            update_message = 'Invalid Input'
        # Here, we assign `update_message' to `message', which means
        # we'll refer to it in our template as `message'
        return render_template('select_time.html', message = update_message)

class add_bid:
    def GET(self):
        return render_template('add_bid.html')

    def POST(self):
        post_params = web.input()
        item_id = post_params['itemID']
        user_id = post_params['userID']
        price = post_params['price']

        result = sqlitedb.addBid(item_id, user_id, price)
        
        return render_template('add_bid.html', message = result)
        
class search:
    def GET(self):
        return render_template('search.html')

    def POST(seft):
        post_params = web.input()
        item_id = post_params['itemID']
        user_id = post_params['userID']
        min_price = post_params['minPrice']
        max_price = post_params['maxPrice']
        status = post_params['status']
        category = post_params['itemCategory']
        description = post_params['itemDescription']
        
        if min_price != '' and max_price != '' and float(min_price) > float(max_price):
            message = 'Max price can not be smaller than min price!'
            return render_template('search.html', message = message)

        search_result = sqlitedb.searchItem(item_id, category, description, user_id, min_price, max_price, status)

        if len(search_result) < 1: 
            return render_template('search.html', search_result = search_result, message = 'No result is found!')
        
        return render_template('search.html', search_result = search_result)

class item:
    def GET(self):
        id = web.input()['id']
        item = sqlitedb.searchItem(id)[0]
        bids = sqlitedb.getBids(id)
        item['Category'] = ', '.join(sqlitedb.getCategory(id))
        
        # check status
        cur_time = sqlitedb.getTime()
        if cur_time < item['Started']:
            item['Status'] = 'Not Started'
        elif cur_time > item['Ends'] or (item['Buy_Price'] is not None and float(item['Currently']) >= float(item['Buy_Price'])):
            item['Status'] = 'Close'
        else:
            item['Status'] = 'Open'

        if len(bids) == 0 or item['Status'] is not 'Close': 
            winner = [] 
        else:
            winner = max(bids, key=lambda bid: bid['Amount'])
        
        return render_template('item.html', item = item, bids = bids, winner = winner)



###########################################################################################
##########################DO NOT CHANGE ANYTHING BELOW THIS LINE!##########################
###########################################################################################

if __name__ == '__main__':
    web.internalerror = web.debugerror
    app = web.application(urls, globals())
    app.add_processor(web.loadhook(sqlitedb.enforceForeignKey))
    app.run()

#!/usr/bin/python

import bottle
import json


bottle.debug(True)

from datetime import datetime

def getSettingsJson(idStr=None):
    return("{name:\"settings\"}")

def getDataJson(idStr=None):
    return("{data:\"dataStr\"}")

##################################################################
# GET requests - return data to seizure detector device
##################################################################
@bottle.get('/settings')
def settings(idStr=None):
    print "settings.get - idStr=%s." % idStr
    bottle.response.content_type='application/javascript'
    return getSettingsJson(idStr)

@bottle.get('/data')
def data(idStr=None):
    print "data.get - idStr=%s." % idStr
    print "response is %s" % getDataJson(idStr)
    bottle.response.content_type='application/javascript'
    return getDataJson(idStr)


####################################################
# PUT requests - seizure detector is sending us data
####################################################
@bottle.put('/settings/<idStr>')
def settings(idStr = None):
    print "settings.put"
    print bottle.request
    print bottle.request.json
    return("settings put")



#######################################################
# POST request - create new risk or mitigation record
#######################################################
@bottle.post('/settings')
def settings():
    print "settings.post"
    return("POST not implemented")


##################################################################
# DELETE requests - delete risk or mitigation
##################################################################
@bottle.delete('/settings/<idStr>')
def risks(idStr = None):
    print "settings.delete %s" % idStr
    return 0


#################################################################
# Entry Page
#################################################################

@bottle.route('/')
#@bottle.route('/index.html')
def entry_page():
    return("index.html")



########################################################
# Run the server
########################################################
#bottle.run(host='localhost', port=8080)
bottle.run(host='0.0.0.0', port=8080)

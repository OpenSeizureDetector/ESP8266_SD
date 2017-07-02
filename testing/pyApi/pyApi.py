#!/usr/bin/python

import bottle
import json
import sqlite3

dbfile="risklog.db"
wwwPath = "/home/graham/risk/www-yo"
maxRiskId = 100

# The parameters that we store as table columns.  Everything else
#  is stored in the JSON blob 'data'
riskParameters = ['id', 'riskNo','type','status','parent','bu','epri','title','data']

bottle.debug(True)

from datetime import datetime

# Convert a sqlite3 row object into a dictionary.
def row2dict(cursor, row):
    retVal = {}
    for (attr, val) in zip((d[0] for d in cursor.description), row) :
        retVal[attr] = val
    return retVal


# get a JSON represenatation of risk 'idStr' or all risks if idStr is not
# specified.
# if idStr is specified, the mitigations associated with the risk
# are included in the returned JSON string.
def getRiskJson(idStr=None):
    print "getRiskJson - idStr=%s." % idStr
    conn = sqlite3.connect(dbfile);
    if (idStr!=None):
        cursor = conn.execute('select * from risks where id=?',(idStr,))
    else:
        cursor = conn.execute('select * from risks')
    retVal = []
    for row in cursor:
        rowd = row2dict(cursor,row)

        if (idStr!=None):
            mitCursor = conn.execute('select * from mitigations where parent=?',(idStr,))
            mitList = []
            for mitRow in mitCursor:
                mitd = row2dict(mitCursor,mitRow)
                mitList.append(mitd)
            rowd['mitigations']=mitList
        retVal.append(rowd)
        

    # if there is only one record, just return the object, not an array
    # of objects.
    if (len(retVal)==1):
        retVal=retVal[0]
    return json.dumps(retVal)



def jsonp(request,d):
    if (request.query.callback):
        return "%s(%s)" % (request.query.callback,d)
    return d

def updateRiskFields(idStr):
    """ update the risk record with id idStr to populate the relevant 
    fields (columns) from the data stored in the 'data' field.
    also sets the modified date field.
    """
    conn = sqlite3.connect(dbfile);

    # First retrieve the record from the database.
    cursor = conn.execute('select * from risks where id=?',(idStr,))
    print cursor
    row = cursor.fetchone()
    if row==None:
        print "Oh no did not return 1 row...."
        return False
    else:
        rowd = row2dict(cursor,row)
        data = rowd['data']
        print data


        # set the fields listed in the riskParameters array from data
        # in the risk data field.
        for key in data:
            if (key in riskParameters):
                sqlStr = "update or fail risks set {keyName}='{keyValue}' where id={id}".\
                         format(keyName=key,
                                keyValue=data[key],
                                id=idStr)
                print sqlStr
                cursor.execute(sqlStr);
                conn.commit()
        sqlStr = "update or fail risks set modified=date('now') where id={id}".\
                 format(id=idStr)
        print sqlStr
        cursor.execute(sqlStr);

        
    return True;

##################################################################
# GET requests - return risk or mitigation data
##################################################################
@bottle.get('/risks')
@bottle.get('/risks/<idStr>')
def risks(idStr=None):
    print "risks.get - idStr=%s." % idStr
    bottle.response.content_type='application/javascript'
    return jsonp(bottle.request,getRiskJson(idStr))


####################################################
# PUT requests - update risk/mitigation data
####################################################
@bottle.put('/risks/<idStr>')
def risks(idStr):
    print "risks.put"
    print bottle.request
    print bottle.request.json
    conn = sqlite3.connect(dbfile);
    cur = conn.cursor()
    sqlStr = "update or fail risks set data='{data}' where id={id}".\
             format(data=json.dumps(bottle.request.json),id=idStr)
    print sqlStr
    cur.execute(sqlStr);
    conn.commit()
    conn.close()
    updateRiskFields(idStr)
    return(jsonp(bottle.request,getRiskJson(idStr)))



#######################################################
# POST request - create new risk or mitigation record
#######################################################
@bottle.post('/risks')
def risks():
    print "risks.post"
    conn = sqlite3.connect(dbfile);
    cur = conn.cursor()
    paramList = []

    # First create a record that just contains 'data'
    print bottle.request
    if 'data' in bottle.request.json:
        sqlStr = "insert into risks (data) values ('{data}')".\
                 format(data=bottle.request.json['data'])
        print sqlStr
        cur.execute(sqlStr)
        conn.commit()
        rowId = cur.lastrowid
        print "rowId=%d" % rowId
    sqlStr = "update or fail risks set created=date('now') where id={id}".\
             format(id=rowId)
    print sqlStr
    cur.execute(sqlStr);

    # set the 'id' field in the data field.
    data['id'] = rowId
    sqlStr = "update or fail risks set data='{data}' where id={id}".\
             format(data=json.dumps(data),id=rowId)
    cur.execute(SqlStr)
    conn.commit()
    conn.close()
    updateRIskFields(rowId)
    return(jsonp(bottle.request,getRiskJson(str(rowId))))


##################################################################
# DELETE requests - delete risk or mitigation
##################################################################
@bottle.delete('/risks/<idStr>')
def risks(idStr):
    print "risks.delete %s" % idStr
    conn = sqlite3.connect(dbfile);
    cur = conn.cursor()
    sqlStr = "delete from risks where id={id}".format(id=idStr)
    print sqlStr
    cur.execute(sqlStr);
    conn.commit()
    conn.close()
    return 0

@bottle.get('/login')
@bottle.post('/login')
def login():
    # Fixme - authenticate user and return a token that is valid for a
    # specific session.
    keyStr = "authentication_key_string";
    return(jsonp(bottle.request,"{msg:'ok',key:'"+keyStr+"'}"))
    
@bottle.get('/logout')
@bottle.post('/logout')
def logout():
    # Fixme - logout the user and delete the token.
    return(jsonp(bottle.request,"{msg:'ok'}"))
    

#################################################################
# Entry Page
#################################################################

@bottle.route('/')
#@bottle.route('/index.html')
def entry_page():
    #bottle.redirect("/static/index.html")
    bottle.redirect("/app/index.html")

#################################################################
# Static files (=main web app)
#################################################################
@bottle.route('/<filename:path>')
def static_files(filename="risk.html"):
    print "static_files - filename = %s." % (filename) 
    return bottle.static_file(filename,root=wwwPath)




########################################################
# Run the server
########################################################
bottle.run(host='localhost', port=8080)

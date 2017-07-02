#!/usr/bin/python


import sqlite3
from datetime import datetime
import random
import json

dbfile="risklog_random.db"

# The parameters that we store as table columns.  Everything else
#  is stored in the JSON blob 'data'
tableParameters = ['id', 'riskNo','type','status','parent','bu','epri','title','data']

buList = ['Hinkley Point B','Hunterston B',
          'Hartlepool','Heysham 1',
          'Heysham 2','Torness',
          'Dungeness B','Sizewell B',
          'CTO']
epriList=['A1','A22','B11','B21','C11','C21','C44','C51','D11','D33','E11','E22']

conn = sqlite3.connect(dbfile)
cur = conn.cursor()

for nRisk in range(1,1000):

    riskNo = "R%00000d" % random.randrange(9000)
    type="R"
    buId = buList[random.randrange(9)]
    epri = epriList[random.randrange(len(epriList))]
    status='open'
    parent=0
    title="Risk %s title" % (riskNo)

    dataObj = {'riskNo':riskNo,
               'type':type,
               'bu':buId,
               'epri':epri,
               'status':status,
               'parent':parent,
               'title':title,
               'qualProbability':random.randrange(60)/10.,
               'qualImpact':random.randrange(60)/10.
    }

    data = {'riskNo':riskNo,'type':type,'bu':buId,'epri':epri,'status':status,'parent':parent,'title':title,'data':json.dumps(dataObj)}

    print nRisk,riskNo,buId,epri,title

    paramList = []

    # First create a record that just contains 'description'
    if ('title' in data):
        sqlStr = "insert into risks (title) values ('{title}')".\
                 format(title=data['title'])
        #print sqlStr
        cur.execute(sqlStr)
        conn.commit()
        rowId = cur.lastrowid
        #print "rowId=%d" % rowId

    for key in data:
        if (key in tableParameters):
            sqlStr = "update or fail risks set {keyName}='{keyValue}' where id={id}".\
                     format(keyName=key,
                            keyValue=data[key],
                            id=rowId)
            #print sqlStr
            cur.execute(sqlStr);
            conn.commit()

            #print "done"        
    sqlStr = "update or fail risks set modified=date('now') where id={id}".\
             format(id=rowId)
    #print sqlStr
    cur.execute(sqlStr);
    sqlStr = "update or fail risks set created=date('now') where id={id}".\
             format(id=rowId)
    #print sqlStr
    cur.execute(sqlStr);
    conn.commit()

    # Now create some mitigations for the risk we have created.
    for mitNo in range(0,random.randrange(4)):
        mitProb = random.randrange(50)*dataObj['qualProbability']/100.
        mitImp = random.randrange(50)*dataObj['qualImpact']/100.
        mitDataObj = {
            'qualProbability':mitProb,
            'qualImpact':mitImp,
            'mitNo':"M%04d" % mitNo,
            'parent':rowId,
            'title':"M%04d Title" % mitNo,
            'benefitDel':random.randrange(100)/100.
            }

        sqlStr = "insert into mitigations (mitNo,parent,title,data,created,modified) values ('{riskNo}:{mitNo}',{parent},'{title}','{data}',date('now'),date('now'))".\
                 format(riskNo=data['riskNo'],mitNo="M%03d"%mitNo,parent=rowId,title="Mit %d title" % mitNo,data=json.dumps(mitDataObj))
        print sqlStr
        cur.execute(sqlStr);
        conn.commit()

conn.close()



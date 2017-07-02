#!/usr/bin/python
""" 
Usage: ngrl2ivor.py [-v] [--ims=imsFname]  NGRLFNAME
Read Excel workbook ngrlFname which should be in the format of a NGRL Excel
download report.  
Convert the data into an ivor risk engine sqlite database

Arguments:
  NGRLFNAME  Excel format NGRL excel download report.

Options:
  -h --help
  -v       verbose mode
  --ims=imsFname     Excel IMS download input filename

"""
from docopt import docopt
import os
import re
from datetime import datetime
#import datetime
import xlrd
import json
import numpy
import sqlite3
import sys
reload(sys)  
sys.setdefaultencoding('utf-8')

class ngrl2ipa(object):
    dataStartRowNo = 2   # row number (counting from zero) of the first
                         # risk data in the ngrl input file.
    imsDataStartRowNo = 3 # row number (starting at zero) of first data line
                          # in ims file.
    imsStartYearColNo = 26

    costPerDay = 0.8  # Lost generation cost in MGPB/day

    dbfile="ngrl2ivor.db"


    # Annual Probabilities and associated qualitative values - used by
    #   self.getQualProb()
    pA = [0,0.01,0.05,0.10,0.20,0.30]
    pQ = [0,1.00,2.00,3.00,4.00,5.00]

    # Actual (Millions of GPB) and qualitative Impacts
    iP = [0,1.00,10.0,50.0,100.,500.]
    iQ = [0,1.00,2.00,3.00,4.00,5.00]


    # The parameters that we store as risk table columns.  Everything else
    #  is stored in the JSON blob 'data'
    riskTableParameters = ['id', 'riskNo','type','status','parent',
                       'bu','epri','title','data']
    mitTableParameters = ['id', 'parent','mitNo','status',
                       'title','data']
    
    
    # columns to populate risk table.
    riskDataTemplate = {'id':None,
                        'riskNo':'riskNo',
                        'type':None,
                        'status':'status',
                        'parent':'parent_riskNo',
                        'bu':'bu',
                        'epri':'epri',
                        'title':'description',
                        'riskOwner':'riskOwner',
                        'qualProbability': None,
                        'qualImpact': None
                        }

    # columns to populate mitigations table.
    mitigationDataTemplate = {'id':None,
                              'mitNo':'mitNo',
                              'parent':None,
                              'riskNo':'riskNo',
                              'status':'status',
                              'bu':'bu',
                              'epri':'epri',
                              'title':'description',
                              'mitigationOwner':'mitigationOwner',
                              'qualProbability': None,
                              'qualImpact': None,
                              'benefitDel':'benefitDel'
                        }


    # Risk Statuses to include in database (All others are ignored)
    riskStatusList = ['Accepted','Addressing','Residual']
    mitStatusList = ['Complete','Committed','In progress']
    
    # Column numbers corresponding to risk data in ngrl input file.
    rColNo = {
        'riskId':0,
        'riskNo':1,
        'parent_riskNo':2,
        'riskOwner':3,
        'status':12,
        'bu':13,
        'epri':27,
        'description':5,
        'prob':26,
        'finImpact':30,
        'units':31,
        'loadRed':32,
        'loadRedDays':33,
        'earlyClose':10,
        'nonFin1':51,
        'nonFin2':52,
        'nonFin3':53,
        'nonFin4':54,
        'nonFin5':55,
        'nonFin6':56,
        'nonFin7':57,
        'repairCost':34,
        'penaltiesCost':35,
        'otherCost':36
    }

    # Column numbers corresponding to mitigation data in ngrl input file
    mColNo = {
        'riskNo':1,
        'riskId_orig':75,
        'mitId_orig':74,
        'mitNo':76,
        'mitigationOwner':77,
        'imsRef':98,
        'description':79,
        'status':81,
        'progress':82,
        'outage':93,
        'benefitDel':84,
        'startDate':85,
        'finishDate':87,
        'prob':100,
        'finImpact':101,
        'units':102,
        'loadRed':103,
        'loadRedDays':104,
        'nonFin1':122,
        'nonFin2':123,
        'nonFin3':124,
        'nonFin4':125,
        'nonFin5':126,
        'nonFin6':127,
        'nonFin7':128,
        'repairCost':105,
        'penaltiesCost':106,
        'otherCost':107,
        'projectCost':89
    }

    imsColNo = {
        'imsRef':0,
        'bu':5,
        'description':6,
        'epri':7,
        'status':18,
        'y1':26,
        'y2':27,
        'y3':28,
        'y4':29,
        'y5':30,
        'y6':31,
        'y7':32,
        'y8':33,
        'y9':34,
        'y10':35
        }

    floatVals = {        # which input data should be converted to floats?
        'benefitTime':1,
        'units':9,
        'finImpact':8,
        'genLoss':10,
        'loadRed':11,
        'loadRedDays':12,
        'repairCost':15,
        'penaltiesCost':16,
        'otherCost':17,
        'prob':16,
        'projectCost':89,
        'y1':1,
        'y2':1,
        'y3':1,
        'y4':1,
        'y5':1,
        'y6':1,
        'y7':1,
        'y8':1,
        'y9':1,
        'y10':1
    }


    def __init__(self,ngrlFname,imsFname=None):
        print "__init__(ngrlFname=%s,imsFname=%s)" \
            % (ngrlFname,imsFname)

        self.nextRiskId = 1
        self.nextMitId = 1
        self.riskArrCombined = []
        self.mitsArrCombined = []

        outDbFname = "ngrl2ivor.db"

        if (not os.path.isfile(ngrlFname)):
            print "ERROR: %s does not exist" % ngrlFname
            exit(-1)
        
        self.importFile(ngrlFname)
        print "Import Complete"

        if (os.path.isfile(imsFname)):
            print "Importing IMS Data File %s" % imsFname
            self.imsArr = self.importIms(imsFname)
        else:
            print "IMS File %s does not exist - ignoring" % imsFname
            self.imsArr = None
        
        #print json.dumps(self.riskArrCombined,indent=4)
        #print json.dumps(self.mitsArrCombined, indent=4)

        print "Writing Database Output...."
        self.writeDb(outDbFname)
        
        print "ngrl2ipa complete"
        print
        

    def getQualProb(self,p):
        """ returns the qualitative probability value from an annual
        probability.
        """
        qualProb = numpy.interp(p,self.pA,self.pQ)
        return qualProb

    def getQualImpact(self,i):
        """ returns the qualitative impact value from a expected financial
        loss in Millions of GBP.
        """
        qualImpact = numpy.interp(i,self.iP,self.iQ)
        return qualImpact


    # Convert a sqlite3 row object into a dictionary.
    def row2dict(self,cursor, row):
        retVal = {}
        for (attr, val) in zip((d[0] for d in cursor.description), row) :
            retVal[attr] = val
        return retVal


    def getRiskIdByRiskNo(self,riskNo):
        """ returns the database id of the row containing risk number
        riskNo.
        """
        conn = sqlite3.connect(self.dbfile)
        cur = conn.cursor()

        # First create a record that just contains 'description'
        sqlStr = "select id from risks where riskNo='{riskNo}'".\
                 format(riskNo = riskNo)
        cur.execute(sqlStr)
        row = cur.fetchone()
        if row==None:
            print "Oh no did not find %s in database" % (riskNo)
            return -1
        else:
            rowd = self.row2dict(cur,row)
            id = rowd['id']
            return id
        
    

    def writeRiskToDb(self,data):
        conn = sqlite3.connect(self.dbfile)
        cur = conn.cursor()

        # First create a record that just contains 'description'
        sqlStr = "insert into risks (title) values (?)"
        #print sqlStr,(data['title'])
        cur.execute(sqlStr,(data['title'],))
        conn.commit()
        rowId = cur.lastrowid
        #print "rowId=%d" % rowId

        # set the correct database id
        data['id'] = rowId
        for key in data:
            if (key in self.riskTableParameters):
                #print key,data[key], type(data[key])
                sqlStr = "update or fail risks set {keyName}=? where id=?".\
                         format(keyName=key)
                cur.execute(sqlStr,(data[key],rowId));

        sqlStr = "update or fail risks set data=? where id=?"
        cur.execute(sqlStr,(json.dumps(data),rowId))
        sqlStr = "update or fail risks set modified=date('now') where id=?"
        #print sqlStr
        cur.execute(sqlStr,(rowId,));
        sqlStr = "update or fail risks set created=date('now') where id=?"
        #print sqlStr
        cur.execute(sqlStr,(rowId,));
        conn.commit()

    def writeMitigationToDb(self,data):
        conn = sqlite3.connect(self.dbfile)
        cur = conn.cursor()

        # First create a record that just contains 'description'
        sqlStr = "insert into mitigations (title) values (?)"
        #print sqlStr,(data['title'])
        cur.execute(sqlStr,(data['title'],))
        conn.commit()
        rowId = cur.lastrowid
        #print "rowId=%d" % rowId

        # set the correct database id
        data['id'] = rowId
        for key in data:
            if (key in self.mitTableParameters):
                #print key,data[key], type(data[key])
                sqlStr = "update or fail mitigations set {keyName}=? where id=?".\
                         format(keyName=key)
                cur.execute(sqlStr,(data[key],rowId));

        sqlStr = "update or fail mitigations set data=? where id=?"
        cur.execute(sqlStr,(json.dumps(data),rowId))
        sqlStr = "update or fail mitigations set modified=date('now') where id=?"
        #print sqlStr
        cur.execute(sqlStr,(rowId,));
        sqlStr = "update or fail mitigations set created=date('now') where id=?"
        #print sqlStr
        cur.execute(sqlStr,(rowId,));
        conn.commit()

        
    def writeDb(self,fname):
        """ xlFname is the template filename
        """
        # Write risks
        rowNo = 0
        print "Writing Risks..."
        for risk in self.riskArrCombined:
            riskDataObj = {}
            #print json.dumps(risk,indent=4)
            for key in self.riskDataTemplate.keys():
                if (self.riskDataTemplate[key]!=None):
                    riskDataObj[key] = risk[self.riskDataTemplate[key]]
                elif (key=='type'):
                    riskDataObj[key] = 'Plant'
                elif (key=='id'):
                    # we do not know the database id until we have put it into
                    # the database so will set this later.
                    riskDataObj[key] = -1
                elif (key=="qualProbability"):
                    riskDataObj[key] = self.getQualProb(risk['prob'])
                elif (key=="qualImpact"):
                    # FIXME - include non-financial cost.
                    impact = risk['finImpact']
                    #impact = risk['loadRed']*risk['loadRedDays']*self.costPerDay \
                     #        + risk['genLoss']*self.costPerDay \
                     #        + risk['repairCost'] \
                     #        + risk['penaltiesCost'] \
                     #        + risk['otherCost']
                    riskDataObj[key] = self.getQualImpact(impact/1e6)
                else:
                    print "Warning - I don't know how to generate key %s." \
                        % (key)
                    
            #print json.dumps(riskDataObj,indent=4)
            self.writeRiskToDb(riskDataObj)
            rowNo += 1
            
        # Write mitigations
        print "Writing Mitigations....."
        for mit in self.mitsArrCombined:
            mitDataObj = {}
            #print json.dumps(risk,indent=4)
            for key in self.mitigationDataTemplate.keys():
                if (self.mitigationDataTemplate[key]!=None):
                    mitDataObj[key] = mit[self.mitigationDataTemplate[key]]
                elif (key=='id'):
                    # we do not know the database id until we have put it into
                    # the database so will set this later.
                    mitDataObj[key] = -1
                elif (key=='parent'):
                    mitDataObj[key] = self.getRiskIdByRiskNo(mit['riskNo'])
                elif (key=="qualProbability"):
                    mitDataObj[key] = self.getQualProb(mit['prob'])
                elif (key=="qualImpact"):
                    # FIXME - include non-financial cost.
                    impact = mit['finImpact']
                    mitDataObj[key] = self.getQualImpact(impact/1e6)
                else:
                    print "Warning - I don't know how to generate key %s." \
                        % (key)
            self.writeMitigationToDb(mitDataObj)


        # Write ims data
        #print "Writing IMS Data....."
        #for imsObj in self.imsArr:
            #print rowNo
        #    for key in self.out_imsColNo.keys():
        #        print key
        #        sheet.write(rowNo,self.out_imsColNo[key],imsObj[key])
        #    rowNo += 1

        # Save the file
        print "Writing %s" % fname
        #book.save(self.xlOutFname)
        
        
    def importFile(self,fpath):
        """ Import the excel file fpath, 
        returning objects (risksObj,mitsObj).
        """
        print "importFile - fpath=%s" % fpath

        book = xlrd.open_workbook(fpath)
        risksArr,mitsArr = self.importRisks(book.sheet_by_index(0))
        #print "risksArr=",risksArr
        self.riskArrCombined.extend(risksArr)
        self.mitsArrCombined.extend(mitsArr)

                
                
        
    def importRisks(self,sheet):
        print "importRisks"
        rowNo = self.dataStartRowNo
        risksArr = []
        mitsArr = []
        while (rowNo<sheet.nrows):
            #print rowNo,sheet.cell(rowNo,0)
            if sheet.cell(rowNo,0).value=='':
                print "empty cell found - end of risks list"
                rowNo = sheet.nrows
            else:
                # First import the risk part of the row
                riskObj = {}
                for key in self.rColNo.keys():
                    value = sheet.cell(rowNo,self.rColNo[key]).value
                    riskObj[key]=value
                    #print type(riskObj[key])
                    if key in self.floatVals:
                        riskObj[key] = self.toFloat(riskObj[key])
                    elif (type(riskObj[key]) is str):
                        #print "converting str to unicode"
                        riskObj[key] = riskObj[key].encode('utf-8')
                    # FIXME - this is a fiddle because original templates do not include
                # refDate.
                riskObj['refDate']="01/01/2016"
                riskObj['riskNo'] = riskObj['riskNo'][0:6] 
                riskObj['repairCost'] = riskObj['repairCost'] + \
                                        riskObj['penaltiesCost'] + \
                                        riskObj['otherCost']
                riskObj['repairCost'] = riskObj['repairCost']/1e6 
                riskObj['nonFin'] = max([
                    riskObj['nonFin1'],
                    riskObj['nonFin2'],
                    riskObj['nonFin3'],
                    riskObj['nonFin4'],
                    riskObj['nonFin5'],
                    riskObj['nonFin6'],
                    riskObj['nonFin7']
                ])
                riskObj['notes'] = " Status="+riskObj['status']

                #print "adding to risksArr"
                # Check to see if this risk is different to the last
                #         one in the array, and append it if different.
                if (riskObj['status'] in self.riskStatusList):
                    if ((len(risksArr)==0) or
                        riskObj['riskId']!=risksArr[-1]['riskId']):
                        risksArr.append(riskObj)
                else:
                    print "Ignoring risk with status %s" % (riskObj['status'])
                # And now read the mitigation part of the row.

                mitObj = {}
                mitObj['bu'] = riskObj['bu']
                mitObj['epri'] = riskObj['epri']
                for key in self.mColNo.keys():
                    val = sheet.cell(rowNo,self.mColNo[key]).value
                    mitObj[key]=val
                    #print rowNo,key,val
                    if key in self.floatVals:
                        mitObj[key] = self.toFloat(mitObj[key])
                    
                mitObj['repairCost'] = mitObj['repairCost'] + \
                                        mitObj['penaltiesCost'] + \
                                        mitObj['otherCost']
                mitObj['repairCost'] = mitObj['repairCost']/1e6 

                mitObj['nonFin'] = max([
                    mitObj['nonFin1'],
                    mitObj['nonFin2'],
                    mitObj['nonFin3'],
                    mitObj['nonFin4'],
                    mitObj['nonFin5'],
                    mitObj['nonFin6'],
                    mitObj['nonFin7']
                ])

                p = mitObj['prob'];
                mitObj['p5'] = p * 5
                mitObj['p10'] = p * 10
                mitObj['p15'] = p * 15

                mitObj['mitNo'] = mitObj['mitNo'][0:] 
                mitObj['riskNo'] = mitObj['riskNo'][0:6] 

                # Treat large load reductions as shutdowns.
                if (mitObj['loadRed'] > 99):
                    mitObj['genLoss'] = mitObj['loadRedDays']
                    mitObj['loadRed']=''
                    mitObj['loadRedDays']=''

                    mitObj['notes'] = mitObj['status']

                    # Calculate work duration
                    refDate = datetime.strptime("01/01/2016",
                                                             "%d/%m/%Y")
                    try:
                        startDate = datetime.strptime(mitObj['startDate'],
                                                             "%d/%m/%Y")
                        finishDate = datetime.strptime(mitObj['finishDate'],
                                                             "%d/%m/%Y")
                        duration1 = (finishDate - startDate).days/365.
                        #duration2 = (finishDate - refDate).days/365.
                        #if (duration2<duration1):
                        #    workDuration = duration2
                        #else:
                        workDuration = duration1
                    except ValueError:
                        # if we don't have a valid date the above will crash
                        # with a ValueError, so just set durations to zero.
                        print "Error Converting startDate or finishDate"
                        print mitObj['startDate'],mitObj['finishDate']
                        workDuration = 0
                    except TypeError:
                        print "Error Converting startDate or finishDate"
                        print mitObj['startDate'],mitObj['finishDate']
                        workDuration = 0
                    mitObj['workDuration'] = workDuration
                    mitObj['benefitTime'] = workDuration
                    

                    # add 1 onto duration to make sure that a project
                    # costs distributed over part years too.
                    duration = int(mitObj['workDuration'])+1 
                    #print "Risk:Mit=%s:%s, StartDate=%s, FinishDate=%s, Duration=%d, ProjectCost=%f" %\
                    #    (mitObj['riskNo'],
                    #     mitObj['mitNo_orig'],
                    #     mitObj['startDate'],
                    #     mitObj['finishDate'],
                    #     duration,
                    #     mitObj['projectCost'])
                    # first zero everything to be sure (not quite sure why
                    # this is necessary....)
                    for i in range(0,10):
                        costKey = "cost_y%d" % (i+1)
                        #print "zeroing...%d,%s" % (i,costKey)
                        mitObj[costKey] = 0
                        fpdKey = "fpd_y%d" % (i+1)
                        #print "zeroing...%d,%s" % (i,fpdKey)
                        mitObj[fpdKey] = 0
                    for i in range(0,duration):
                        costKey = "cost_y%d" % (i+1)
                        #print i,costKey
                        mitObj[costKey] = mitObj['projectCost']/duration/1.0e6
                
                #if (mitObj['riskNo']!=''):
                if ((riskObj['status'] in self.riskStatusList) and \
                   (mitObj['status'] in self.mitStatusList)):
                    mitsArr.append(mitObj)
            print "rowNo=%d" % rowNo
            rowNo += 1
        return risksArr,mitsArr
        
    def importIms(self,imsFname):
        """ import an IMS excel download file into an array of objects
        """
        print "importIms - imsFname=%s" % imsFname
        imsArr = []
        book = xlrd.open_workbook(imsFname)
        sheet = book.sheet_by_index(0)
        rowNo = self.imsDataStartRowNo

        dataStartYear = sheet.cell(self.imsDataStartRowNo-1,self.imsStartYearColNo).value
        print "dataStartYear = %s" % dataStartYear

        # Find column number of 'in plan' field
        colNo = 0
        inPlanColNo = -1
        while (colNo <sheet.ncols):
            if (sheet.cell(self.imsDataStartRowNo-1,colNo).value == "Included in 5 year plan"):
                inPlanColNo = colNo
            colNo += 1
        print "inPlanColNo = %d" % inPlanColNo
        self.imsColNo['inPlan'] = inPlanColNo
        
        # read each line in turn
        while (rowNo < sheet.nrows):
                        #print rowNo,sheet.cell(rowNo,0)
            if sheet.cell(rowNo,0).value=='':
                print "empty cell found - end of IMS data"
                rowNo = sheet.nrows
            else:
                # First import the risk part of the row
                imsObj = {}
                for key in self.imsColNo.keys():
                    value = sheet.cell(rowNo,self.imsColNo[key]).value
                    imsObj[key]=value
                    if key in self.floatVals:
                        imsObj[key] = self.toFloat(imsObj[key])
                # split description off epri code
                imsObj['epri'] = imsObj['epri'].split(' ')[0]
                r,m = self.getMitIdByImsRef(imsObj['imsRef'])
                imsObj['riskNo'] = r
                imsObj['mitNo'] = m
                imsObj['startYear'] = dataStartYear
                imsObj['notes'] = "status="+ imsObj['status'] \
                                  + " plan="+imsObj['inPlan']
                imsObj['y1'] = imsObj['y1']/1000.
                imsObj['y2'] = imsObj['y2']/1000.
                imsObj['y3'] = imsObj['y3']/1000.
                imsObj['y4'] = imsObj['y4']/1000.
                imsObj['y5'] = imsObj['y5']/1000.
                imsObj['y6'] = imsObj['y6']/1000.
                imsObj['y7'] = imsObj['y7']/1000.
                imsObj['y8'] = imsObj['y8']/1000.
                imsObj['y9'] = imsObj['y9']/1000.
                imsObj['y10'] = imsObj['y10']/1000.
                imsObj = self.setImsStartDate(imsObj)
                imsArr.append(imsObj)
                rowNo += 1
        #print imsArr
        return imsArr

    def setImsStartDate(self,imsObj):
        """ set the start date of the work based on the spend profile
        and adjust spend profile to match.
        """
        print "setImsStartDate() - %s" % imsObj['imsRef']
        if self.getImsCost(imsObj) == 0:
            print "zero cost scheme - ignoring"
        else:
            print "startYear=%d" % imsObj['startYear']
            while (imsObj['y1']==0):
                imsObj = self.moveImsCostProfile(imsObj)
                print "startYear=%d" % imsObj['startYear']
        imsObj['startDate'] = "01/01/%d" % imsObj['startYear']

        imsObj['workDuration'] = 0
        if (imsObj['y1'] > 0): imsObj['workDuration'] = 1
        if (imsObj['y2'] > 0): imsObj['workDuration'] = 2
        if (imsObj['y3'] > 0): imsObj['workDuration'] = 3
        if (imsObj['y4'] > 0): imsObj['workDuration'] = 4
        if (imsObj['y5'] > 0): imsObj['workDuration'] = 5
        if (imsObj['y6'] > 0): imsObj['workDuration'] = 6
        if (imsObj['y7'] > 0): imsObj['workDuration'] = 7
        if (imsObj['y8'] > 0): imsObj['workDuration'] = 8
        if (imsObj['y9'] > 0): imsObj['workDuration'] = 9
        if (imsObj['y10'] > 0): imsObj['workDuration'] = 10
        imsObj['benefitTime'] = imsObj['workDuration']
        return imsObj

    def moveImsCostProfile(self,imsObj):
        """ Move the y1,y2 etc. cost profile to the left, and increment start
        year """
        imsObj['startYear'] = imsObj['startYear'] + 1
        imsObj['y1'] = imsObj['y2']
        imsObj['y2'] = imsObj['y3']
        imsObj['y3'] = imsObj['y4']
        imsObj['y4'] = imsObj['y5']
        imsObj['y5'] = imsObj['y6']
        imsObj['y6'] = imsObj['y7']
        imsObj['y7'] = imsObj['y8']
        imsObj['y8'] = imsObj['y9']
        imsObj['y9'] = imsObj['y10']
        imsObj['y10'] = 0
        return imsObj
        
    def getImsCost(self,imsObj):
        """ return the total cost of the ims scheme in imsObj"""
        cost = imsObj['y1'] \
               + imsObj['y2'] \
               + imsObj['y3'] \
               + imsObj['y4'] \
               + imsObj['y5'] \
               + imsObj['y6'] \
               + imsObj['y7'] \
               + imsObj['y8'] \
               + imsObj['y9'] \
               + imsObj['y10']
        return cost
    
    def toFloat(self,valStr):
        """ Convert the value valStr to a floating point number
        """
        #print valStr
        if valStr=="":
            val = 0
        else:
            # If we have been passed a string, strip off any extra '%'
            # characters.
            if ((type(valStr) is unicode) or
                (type(valStr) is str)):
                #print "stripping % from string"
                #valStr = valStr.split('%')[0]
                #valStr = valStr.translate(None,'%, ')
                valStr = re.sub('[%, ]','',valStr)
            try:
                val = float(valStr)
            except:
                val = 0
                print "error converting %s to float" % valStr
        return val
            
    def getNewRiskId(self,id,riskArr):
        for risk in riskArr:
            if risk['id_orig']==id:
                return risk['id']
        return None

    def getMitIdByImsRef(self,imsRef):
        """ return the mitigation reference (risk,mit) of the investment
        with ims reference imsRef.
        """
        riskNo = -1
        mitNo = -1

        for mitObj in self.mitsArrCombined:
            if (mitObj['imsRef'].find(str(int(imsRef)))!=-1):
                print "Found Mitigation Link - imsRef=%s, mitObj=%s" % (str(int(imsRef)),mitObj['imsRef'])
                riskNo = mitObj['riskNo']
                mitNo = mitObj['mitNo_orig']
                return (riskNo,mitNo)
        print "getMitIdByImsRef() - Failed to find Mitigation link for IMS Ref %s" % (str(int(imsRef)))
        return(-1,-1)
    
if __name__ == '__main__':
    args = docopt(__doc__)
    print(args)

    if (args['--ims'] != None):
        imsFile = args['--ims']
    else:
        imsFile = ""

        

    ngrlFname = args['NGRLFNAME']

    print "ngrl2ipa - ngrlFname = %s" \
        % (ngrlFname)

    x2j = ngrl2ipa(ngrlFname,imsFile)

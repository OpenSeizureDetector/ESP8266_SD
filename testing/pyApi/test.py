#!/usr/bin/python
"""
Tests for pyApi
"""
import unittest
import json


class TestJSON(unittest.TestCase):
    def testJson(self):
        riskObj = {"id": 1, "epri": "B11", "bu": "HRA"}
        print riskObj
        riskJSON = json.dumps(riskObj)
        print riskJSON
        riskObj2 = json.loads(riskJSON)
        self.assertEqual(riskObj2['id'], 1)
        self.assertEqual(riskObj2['epri'], "B11")

    def testJsonUnicode(self):
        riskObj = {u"id": 1, u"epri": u"B11", u"bu": u"HRA"}
        print riskObj
        riskJSON = json.dumps(riskObj)
        print riskJSON
        riskObj2 = json.loads(riskJSON)
        self.assertEqual(riskObj2['id'], 1)
        self.assertEqual(riskObj2['epri'], "B11")
        

if __name__ == '__main__':
    unittest.main()

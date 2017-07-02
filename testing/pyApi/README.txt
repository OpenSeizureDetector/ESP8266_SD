pyAPI README
============

This folder contains a simple python based web server to store
and retrieve risks and mitigations in an sqlite database.

The schema is very simple - very little data is stored in database
columns with most data stored as a single json blob.

The advantage of this is that we can change the database schema
easily during development.
The disadvantage is that it is likely to be slower because we need
to interpret the json blob for each risk or mitigation to apply
filters on fields that are not stored as rows.
For this reason the following data is stored as database table rows:

ID - unique id of the risk. (integer)
riskNo - NGRL risk number   (String)
type - R or M for risk or mitigation (String)
bu - business unit   (String)
epri - epri code   (String)
title - risk title   (String)
parent - the unique ID of the parent.
data - complete data for risk/mitigation as a json blob.

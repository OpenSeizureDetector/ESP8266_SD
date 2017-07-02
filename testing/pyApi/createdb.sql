drop table if exists  users;
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    email VARCHAR(255) NOT NULL,
    password VARCHAR(255) NOT NULL,
    created DATETIME,
    modified DATETIME
);

drop table if exists risks;
create table risks (
       id INT AUTO_INCREMENT PRIMARY KEY,
       bu varchar(3),
       epri varchar(10),
       description varchar(100),
       riskType varchar(20),
       refDate DATETIME,
       p5 float,
       p10 float,
       p15 float,
       genLoss float,
       loadRed float,
       loadRedDays float,
       repairCost float,
       nonFinCode varchar(10),
       riskData varchar(512),
       notes varchar(512),
       created DATETIME,
       modified DATETIME
);
insert into risks (id, bu, epri,description,riskType,
       refDate,p5,p10,p15,
       genLoss,loadRed,loadRedDays,repairCost,nonFinCode,
       riskData,notes,
       created, modified) values
       (1,"HRA","B11","risk1","Operational",
       "2016-01-01",5,10,15,
       20,0,0,1,"",
       "{}","",
       now(),now());

drop table if exists mitigations;
create table mitigations (
       id int auto_increment primary key,
       risk_ref int,
       mitigation_type int,
       description varchar(100),
       mitigation_data varchar(512),
       created DATETIME,
       modified DATETIME
);

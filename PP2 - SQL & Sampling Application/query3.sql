with WeeklySales(weeklysales) as
       (
         select sum(weeklysales), isholiday
         from sales
                join holidays on sales.weekdate = holidays.weekdate
         group by sales.weekdate, isholiday
       ),
     HolidayWeeklySales(weeklysales) AS
       (
         select *
         from WeeklySales
         where isholiday = true
       ),
     NonHolidayWeeklySales(weeklysales) AS
       (
         select *
         from WeeklySales
         where isholiday = false
       )
select count(*)
from NonHolidayWeeklySales
where NonHolidayWeeklySales.weeklysales >
      (select avg(weeklysales) from HolidayWeeklySales);

--  count 
-- -------
--      8
-- (1 row)
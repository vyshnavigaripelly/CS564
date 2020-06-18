with StoreDeptSales(store, dept, sales) AS
       (
         select store, dept, sum(weeklysales)
         from sales
         group by store, dept
       ),
     DeptNormSales(dept, normsales) AS
       (
         select dept, sum(sales / size)
         from StoreDeptSales
                join stores on stores.store = StoreDeptSales.store
         group by dept
       )
select dept, normsales
from DeptNormSales
order by normsales desc
limit 10;

--  dept |    normsales     
-- ------+------------------
--    92 | 4128.35295214021
--    38 |  4080.2108840889
--    95 | 3879.83508746934
--    90 | 2567.52581960705
--    40 | 2400.34804213186
--     2 | 2232.72934175439
--    72 | 2191.77413470508
--    91 | 1791.72821711059
--    94 | 1747.77835055544
--    13 | 1620.50958705405
-- (10 rows)
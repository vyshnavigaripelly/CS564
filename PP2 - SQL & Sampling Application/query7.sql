with DepartSale(store,
       dept,
       weeklysales) as (select store, dept, sum(weeklysales) as sum from sales group by store, dept),
     StoreSale(store,
       weeklysales) as (select store, sum(weeklysales) from DepartSale group by store),
     DepartSaleContribution(store,
       dept,
       contribution) AS (select StoreSale.store, DepartSale.dept, DepartSale.weeklysales / StoreSale.weeklysales
                         from StoreSale
                                join DepartSale on StoreSale.store = DepartSale.store),
     DepartContributionAvg(dept,
       avg) as (select dept, avg(contribution) from DepartSaleContribution group by dept),
     DepartWant(dept) as (select dept
                          from DepartSaleContribution
                          where contribution > 0.05
                          group by dept
                          having count(dept) > 3)
select DepartWant.dept, avg
from DepartWant
       join DepartContributionAvg on DepartWant.dept = DepartContributionAvg.dept;

--  dept |        avg         
-- ------+--------------------
--    92 | 0.0730967374311553
--    40 | 0.0441973057058122
--    72 | 0.0420093151980281
--    95 |   0.06952508100205
--    91 | 0.0313699989941799
--    90 | 0.0449520758870575
--    93 | 0.0254024096970022
--     2 | 0.0410644161204497
--    38 | 0.0727544363174174
--    94 | 0.0304081379655852
-- (10 rows)
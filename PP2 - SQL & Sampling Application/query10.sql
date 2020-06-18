with type_sale as (select type,
                          extract(year from weekdate)    as year,
                          extract(quarter from weekdate) as quarters,
                          sum(weeklysales)               as sum
                   from sales
                          join stores on sales.store = stores.store
                   group by type, year, quarters),

     summary (year, quarter, A_sale, B_sale, C_sale) as (
       select A.year, A.quarters, A.sum, B.sum, C.sum
       from type_sale as A
              join (type_sale as B join type_sale as C on B.year = C.year and (B.quarters = C.quarters))
                   on A.year = B.year and (A.quarters = B.quarters)
       where A.type = 'A'
         and B.type = 'B'
         and C.type = 'C')

select *
from summary
union all
select year, NULL, sum(A_sale), sum(B_sale), sum(C_sale)
from summary
group by year
order by year, quarter nulls last;

--  year | quarter |   a_sale    |   b_sale    |   c_sale    
-- ------+---------+-------------+-------------+-------------
--  2010 |       1 | 2.38155e+08 | 1.11852e+08 | 2.22458e+07
--  2010 |       2 | 3.90789e+08 |  1.8321e+08 | 3.63699e+07
--  2010 |       3 | 3.82693e+08 | 1.78504e+08 | 3.62904e+07
--  2010 |       4 | 4.53791e+08 | 2.16413e+08 |  3.8572e+07
--  2010 |         | 1.46543e+09 | 6.89978e+08 | 1.33478e+08
--  2011 |       1 | 3.41851e+08 | 1.53903e+08 | 3.36366e+07
--  2011 |       2 | 3.85807e+08 | 1.75557e+08 | 3.65837e+07
--  2011 |       3 | 4.13364e+08 | 1.87497e+08 | 3.84974e+07
--  2011 |       4 | 4.37185e+08 | 2.07162e+08 | 3.71554e+07
--  2011 |         | 1.57821e+09 | 7.24119e+08 | 1.45873e+08
--  2012 |       1 | 3.81451e+08 | 1.72499e+08 | 3.85173e+07
--  2012 |       2 | 3.98116e+08 | 1.81932e+08 | 3.82483e+07
--  2012 |       3 | 3.89167e+08 | 1.78201e+08 | 3.76368e+07
--  2012 |       4 |  1.1864e+08 | 5.39715e+07 | 1.17501e+07
--  2012 |         | 1.28737e+09 | 5.86604e+08 | 1.26152e+08
-- (15 rows)
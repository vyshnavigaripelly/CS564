with StoreSale(store, sales) as (
  select store, sum(WeeklySales)
  from sales
         join holidays on sales.weekdate = holidays.weekdate
  where holidays.isholiday
  group by store
)
select store, sales
from StoreSale
where sales in (select max(sales)
                from StoreSale
                union
                select min(sales)
                from StoreSale);

--  store |    sales    
-- -------+-------------
--     20 | 2.24904e+07
--     33 | 2.62595e+06
-- (2 rows)
with YearMonthSales(month, year, store, dept, weeklysales) AS
       (
         select extract(month from weekdate),
                extract(year from weekdate),
                store,
                dept,
                weeklysales
         from sales
       ),
     YearMonthCount(year, month) AS 
      (
         select year, count(distinct month) 
         from YearMonthSales
         group by year
      ),
     NonZeroSales(month, year, store, dept, weeklysales) AS
       (
         select *
         from YearMonthSales
         where weeklysales <> 0
       ),
     DeptMonthCount(year, store, dept, month_count) AS
       (
         select year, store, dept, count(distinct month)
         from NonZeroSales
         group by store, dept, year
       ),
     DeptFullMonth(year, store, dept) AS
       (
         select DeptMonthCount.year, store, dept
         from DeptMonthCount join YearMonthCount on DeptMonthCount.year = YearMonthCount.year
         where DeptMonthCount.month_count = YearMonthCount.month
       ),
     StoreFullMonth(store, year, dept_count) AS
       (
         select store, year, count(distinct dept)
         from DeptFullMonth
         group by store, year
       ),
     StoreFullMonthInAnyYear(store, dept_count) AS
       (
         select store, max(dept_count)
         from StoreFullMonth
         group by store
       ),
     StoreDeptCount (store, dept_count) as
       (
         select store, count(distinct dept)
         from sales
         group by store
       )
select StoreFullMonthInAnyYear.store
from StoreFullMonthInAnyYear
       join StoreDeptCount on StoreFullMonthInAnyYear.store = StoreDeptCount.store
where StoreDeptCount.dept_count = StoreFullMonthInAnyYear.dept_count;

--  store 
-- -------
-- (0 rows)
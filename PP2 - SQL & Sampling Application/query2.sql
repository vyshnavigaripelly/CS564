with StoreMaxFuelPrice AS
       (
         select max(fuelprice) as max_fuel_price,
                max(unemploymentrate) as max_unemployment_rate,
                store
         from temporaldata
         group by store
       )
select store
from StoreMaxFuelPrice
where max_fuel_price < 4 and max_unemployment_rate > 10;

--  store 
-- -------
--     34
--     43
-- (2 rows)
select count(item_id) from (select item_id, count(category_id) as count from ItemCategory group by item_id) where count = 4;



-- select count(distinct bid_id) from ItemBid
-- where item_id in (
--   select item_id
--   from (
--     select item_id, count(category_id) as count
--     from ItemCategory
--     group by item_id
--   )
--   where count = 4
-- );
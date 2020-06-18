select count(user_id)
from User
where user_id in (
    SELECT seller_id
    FROM ItemSeller
) and rating > 1000
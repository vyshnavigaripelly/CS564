select count(distinct category_id) 
    from ItemCategory
    where item_id in(
        select item_id 
        from ItemBid 
        where bid_id in (
            select bid_id 
            from Bid 
            where amount > 100
        )
)
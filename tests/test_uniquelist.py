import pytest

from hotteok import UniqueList


# --- construction ---------------------------------------------------------

def test_empty_init():
    ul = UniqueList()
    assert len(ul) == 0
    assert list(ul) == []

def test_init_from_list():
    ul = UniqueList([1, 2, 3])
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]

def test_init_dedupes_input():
    ul = UniqueList([1, 2, 2, 3, 1])
    assert list(ul) == [1, 2, 3]
    assert len(ul) == 3

def test_init_from_generator():
    ul = UniqueList(x * x for x in range(5))
    assert list(ul) == [0, 1, 4, 9, 16]
    assert len(ul) == 5

def test_init_from_tuple():
    ul = UniqueList(("a", "b", "c"))
    assert list(ul) == ["a", "b", "c"]
    assert len(ul) == 3


# --- append ---------------------------------------------------------------

def test_append_new_item():
    ul = UniqueList()
    assert len(ul) == 0
    ul.append(1)
    assert len(ul) == 1
    ul.append(2)
    assert len(ul) == 2
    assert list(ul) == [1, 2]

def test_append_duplicate_is_noop():
    ul = UniqueList([1, 2])
    assert len(ul) == 2
    ul.append(1)
    assert len(ul) == 2
    assert list(ul) == [1, 2]

def test_append_unhashable_raises():
    ul = UniqueList()
    with pytest.raises(TypeError):
        ul.append([1, 2])
    assert len(ul) == 0


# --- extend ---------------------------------------------------------------

def test_extend_adds_new_items():
    ul = UniqueList([1])
    assert len(ul) == 1
    ul.extend([2, 3, 4])
    assert len(ul) == 4
    assert list(ul) == [1, 2, 3, 4]

def test_extend_skips_duplicates():
    ul = UniqueList([1, 2])
    assert len(ul) == 2
    ul.extend([2, 3, 1, 4])
    assert len(ul) == 4
    assert list(ul) == [1, 2, 3, 4]


# --- pop ------------------------------------------------------------------

def test_pop_default_is_last():
    ul = UniqueList([1, 2, 3])
    assert len(ul) == 3
    assert ul.pop() == 3
    assert len(ul) == 2
    assert list(ul) == [1, 2]

def test_pop_by_index():
    ul = UniqueList([1, 2, 3])
    assert len(ul) == 3
    assert ul.pop(0) == 1
    assert len(ul) == 2
    assert list(ul) == [2, 3]

def test_pop_negative_index():
    ul = UniqueList([1, 2, 3])
    assert len(ul) == 3
    assert ul.pop(-2) == 2
    assert len(ul) == 2
    assert list(ul) == [1, 3]

def test_pop_empty_raises():
    ul = UniqueList()
    with pytest.raises(IndexError):
        ul.pop()
    assert len(ul) == 0

def test_pop_out_of_range_raises():
    ul = UniqueList([1, 2])
    with pytest.raises(IndexError):
        ul.pop(5)
    assert len(ul) == 2

def test_pop_allows_re_adding_same_value():
    ul = UniqueList([1, 2, 3])
    ul.pop(1)                   # removes 2
    assert len(ul) == 2
    ul.append(2)                # should work again
    assert len(ul) == 3
    assert list(ul) == [1, 3, 2]


# --- index ----------------------------------------------------------------

def test_index_returns_position():
    ul = UniqueList(["a", "b", "c"])
    assert ul.index("b") == 1
    assert len(ul) == 3

def test_index_missing_raises():
    ul = UniqueList([1, 2])
    with pytest.raises(ValueError):
        ul.index(99)
    assert len(ul) == 2


# --- index fast path ------------------------------------------------------

def test_index_after_insert():
    ul = UniqueList([1, 2, 3, 4])
    ul.insert(1, 99)   # [1, 99, 2, 3, 4]
    assert ul.index(1) == 0
    assert ul.index(99) == 1
    assert ul.index(2) == 2
    assert ul.index(3) == 3
    assert ul.index(4) == 4

def test_index_after_pop():
    ul = UniqueList([1, 2, 3, 4])
    ul.pop(1)          # [1, 3, 4]
    assert ul.index(1) == 0
    assert ul.index(3) == 1
    assert ul.index(4) == 2

def test_index_after_pop_last_is_noop_reindex():
    ul = UniqueList([1, 2, 3, 4])
    ul.pop()           # [1, 2, 3]
    assert ul.index(1) == 0
    assert ul.index(2) == 1
    assert ul.index(3) == 2

def test_index_after_remove():
    ul = UniqueList([1, 2, 3, 4])
    ul.remove(2)       # [1, 3, 4]
    assert ul.index(1) == 0
    assert ul.index(3) == 1
    assert ul.index(4) == 2

def test_index_after_delitem():
    ul = UniqueList([1, 2, 3, 4])
    del ul[0]          # [2, 3, 4]
    assert ul.index(2) == 0
    assert ul.index(3) == 1
    assert ul.index(4) == 2

def test_index_after_setitem():
    ul = UniqueList([1, 2, 3])
    ul[1] = 99         # [1, 99, 3]
    assert ul.index(1) == 0
    assert ul.index(99) == 1
    assert ul.index(3) == 2

def test_index_unhashable_raises():
    ul = UniqueList([1, 2, 3])
    with pytest.raises(TypeError):
        ul.index([1, 2])


# --- insert ---------------------------------------------------------------

def test_insert_middle():
    ul = UniqueList([1, 3])
    assert len(ul) == 2
    ul.insert(1, 2)
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]

def test_insert_at_end():
    ul = UniqueList([1, 2])
    ul.insert(len(ul), 3)
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]

def test_insert_clamps_large_index():
    ul = UniqueList([1, 2])
    ul.insert(999, 3)
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]

def test_insert_clamps_very_negative():
    ul = UniqueList([1, 2])
    ul.insert(-999, 0)
    assert len(ul) == 3
    assert list(ul) == [0, 1, 2]

def test_insert_duplicate_raises():
    ul = UniqueList([1, 2, 3])
    with pytest.raises(ValueError):
        ul.insert(0, 2)
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]


# --- remove ---------------------------------------------------------------

def test_remove_existing():
    ul = UniqueList([1, 2, 3])
    assert len(ul) == 3
    ul.remove(2)
    assert len(ul) == 2
    assert list(ul) == [1, 3]

def test_remove_missing_raises():
    ul = UniqueList([1, 2])
    with pytest.raises(ValueError):
        ul.remove(99)
    assert len(ul) == 2


# --- copy -----------------------------------------------------------------

def test_copy_is_independent():
    ul = UniqueList([1, 2, 3])
    dup = ul.copy()
    assert len(ul) == 3
    assert len(dup) == 3
    dup.append(4)
    assert len(ul) == 3
    assert len(dup) == 4
    assert list(ul) == [1, 2, 3]
    assert list(dup) == [1, 2, 3, 4]


# --- iteration & len ------------------------------------------------------

def test_len_tracks_mutations():
    ul = UniqueList()
    assert len(ul) == 0
    ul.append(1)
    assert len(ul) == 1
    ul.extend([2, 3])
    assert len(ul) == 3
    ul.pop()
    assert len(ul) == 2

def test_iteration_yields_in_order():
    ul = UniqueList([10, 20, 30])
    collected = []
    for x in ul:
        collected.append(x)
    assert collected == [10, 20, 30]
    assert len(ul) == 3

def test_iteration_is_repeatable():
    ul = UniqueList([1, 2, 3])
    assert list(ul) == [1, 2, 3]
    assert list(ul) == [1, 2, 3]   # second pass works
    assert len(ul) == 3

def test_unpacking():
    ul = UniqueList([1, 2, 3])
    a, b, c = ul
    assert (a, b, c) == (1, 2, 3)
    assert len(ul) == 3


# --- tolist ---------------------------------------------------------------

def test_tolist_matches_iteration():
    ul = UniqueList([1, 2, 3])
    assert ul.tolist() == list(ul) == [1, 2, 3]

def test_tolist_empty():
    ul = UniqueList()
    assert ul.tolist() == []

def test_tolist_returns_plain_list():
    ul = UniqueList([1, 2, 3])
    result = ul.tolist()
    assert type(result) is list

def test_tolist_returns_independent_copy():
    ul = UniqueList([1, 2, 3])
    result = ul.tolist()
    result.append(999)
    result[0] = 0
    assert ul.tolist() == [1, 2, 3]
    assert len(ul) == 3

def test_tolist_returns_fresh_list_each_call():
    ul = UniqueList([1, 2, 3])
    assert ul.tolist() is not ul.tolist()

def test_tolist_reflects_mutations():
    ul = UniqueList([1, 2, 3])
    ul.append(4)
    assert ul.tolist() == [1, 2, 3, 4]
    ul.pop(0)
    assert ul.tolist() == [2, 3, 4]
    del ul[0]
    assert ul.tolist() == [3, 4]


# --- contains -------------------------------------------------------------

def test_contains_existing_item():
    ul = UniqueList([1, 2, 3])
    assert 2 in ul
    assert len(ul) == 3

def test_contains_missing_item():
    ul = UniqueList([1, 2, 3])
    assert 99 not in ul
    assert len(ul) == 3

def test_contains_empty():
    ul = UniqueList()
    assert 1 not in ul
    assert len(ul) == 0

def test_contains_after_append():
    ul = UniqueList([1, 2])
    assert 3 not in ul
    assert len(ul) == 2
    ul.append(3)
    assert 3 in ul
    assert len(ul) == 3

def test_contains_after_remove():
    ul = UniqueList([1, 2, 3])
    assert 2 in ul
    assert len(ul) == 3
    ul.remove(2)
    assert 2 not in ul
    assert len(ul) == 2

def test_contains_after_pop():
    ul = UniqueList([1, 2, 3])
    ul.pop(0)
    assert 1 not in ul
    assert 2 in ul
    assert len(ul) == 2

def test_contains_heterogeneous_types():
    ul = UniqueList([1, "a", (1, 2)])
    assert 1 in ul
    assert "a" in ul
    assert (1, 2) in ul
    assert "1" not in ul
    assert len(ul) == 3

def test_contains_unhashable_raises():
    ul = UniqueList([1, 2, 3])
    with pytest.raises(TypeError):
        [1, 2] in ul
    assert len(ul) == 3


# --- __getitem__ ---------------------------------------------------------

def test_getitem_by_index():
    ul = UniqueList([10, 20, 30])
    assert ul[0] == 10
    assert ul[1] == 20
    assert ul[2] == 30
    assert len(ul) == 3

def test_getitem_negative_index():
    ul = UniqueList([10, 20, 30])
    assert ul[-1] == 30
    assert ul[-2] == 20
    assert ul[-3] == 10
    assert len(ul) == 3

def test_getitem_out_of_range_raises():
    ul = UniqueList([1, 2])
    with pytest.raises(IndexError):
        ul[5]
    with pytest.raises(IndexError):
        ul[-5]
    assert len(ul) == 2

def test_getitem_empty_raises():
    ul = UniqueList()
    with pytest.raises(IndexError):
        ul[0]
    assert len(ul) == 0

def test_getitem_non_integer_raises():
    ul = UniqueList([1, 2, 3])
    with pytest.raises(TypeError):
        ul["a"]
    assert len(ul) == 3


# --- __setitem__ ---------------------------------------------------------

def test_setitem_replaces_value():
    ul = UniqueList([1, 2, 3])
    ul[1] = 20
    assert len(ul) == 3
    assert list(ul) == [1, 20, 3]
    assert 2 not in ul
    assert 20 in ul

def test_setitem_negative_index():
    ul = UniqueList([1, 2, 3])
    ul[-1] = 99
    assert len(ul) == 3
    assert list(ul) == [1, 2, 99]
    assert 3 not in ul

def test_setitem_duplicate_raises():
    ul = UniqueList([1, 2, 3])
    with pytest.raises(ValueError):
        ul[0] = 2   # 2 already exists elsewhere
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]   # unchanged (rollback)

def test_setitem_same_value_at_same_index():
    ul = UniqueList([1, 2, 3])
    ul[0] = 1       # assigning same value should be a no-op-ish success
    assert len(ul) == 3
    assert list(ul) == [1, 2, 3]

def test_setitem_out_of_range_raises():
    ul = UniqueList([1, 2])
    with pytest.raises(IndexError):
        ul[10] = 99
    assert len(ul) == 2
    assert list(ul) == [1, 2]

def test_setitem_allows_replaced_value_to_be_reused():
    ul = UniqueList([1, 2, 3])
    ul[1] = 20       # 2 is released
    assert len(ul) == 3
    ul.append(2)     # 2 can be added back at a new position
    assert len(ul) == 4
    assert list(ul) == [1, 20, 3, 2]

def test_setitem_preserves_len():
    ul = UniqueList([1, 2, 3])
    ul[0] = 10
    assert len(ul) == 3


# --- __delitem__ ---------------------------------------------------------

def test_delitem_removes_value():
    ul = UniqueList([1, 2, 3])
    del ul[1]
    assert len(ul) == 2
    assert list(ul) == [1, 3]
    assert 2 not in ul

def test_delitem_negative_index():
    ul = UniqueList([1, 2, 3])
    del ul[-1]
    assert len(ul) == 2
    assert list(ul) == [1, 2]
    assert 3 not in ul

def test_delitem_out_of_range_raises():
    ul = UniqueList([1, 2])
    with pytest.raises(IndexError):
        del ul[10]
    assert len(ul) == 2
    assert list(ul) == [1, 2]

def test_delitem_empty_raises():
    ul = UniqueList()
    with pytest.raises(IndexError):
        del ul[0]
    assert len(ul) == 0

def test_delitem_allows_re_adding():
    ul = UniqueList([1, 2, 3])
    del ul[0]
    assert len(ul) == 2
    ul.append(1)
    assert len(ul) == 3
    assert list(ul) == [2, 3, 1]

def test_delitem_updates_len():
    ul = UniqueList([1, 2, 3])
    del ul[0]
    assert len(ul) == 2
    del ul[0]
    assert len(ul) == 1


# --- __reverse__ ---------------------------------------------------------

def test_reverse():
    ul = UniqueList([1, 3, 2, 4])
    assert list(reversed(ul)) == [4, 2, 3, 1]
    assert len(ul) == 4

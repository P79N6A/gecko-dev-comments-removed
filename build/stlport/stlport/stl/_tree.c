





























#ifndef _STLP_TREE_C
#define _STLP_TREE_C

#ifndef _STLP_INTERNAL_TREE_H
#  include <stl/_tree.h>
#endif

#if defined (_STLP_DEBUG)
#  define _Rb_tree _STLP_NON_DBG_NAME(Rb_tree)
#endif



#if defined (_STLP_NESTED_TYPE_PARAM_BUG)
#  define __iterator__  _Rb_tree_iterator<_Value, _STLP_HEADER_TYPENAME _Traits::_NonConstTraits>
#  define __size_type__ size_t
#  define iterator __iterator__
#else
#  define __iterator__  _STLP_TYPENAME_ON_RETURN_TYPE _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc>::iterator
#  define __size_type__  _STLP_TYPENAME_ON_RETURN_TYPE _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc>::size_type
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

#if defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)

template <class _Dummy> void _STLP_CALL
_Rb_global<_Dummy>::_Rotate_left(_Rb_tree_node_base* __x,
                                 _Rb_tree_node_base*& __root) {
  _Rb_tree_node_base* __y = __x->_M_right;
  __x->_M_right = __y->_M_left;
  if (__y->_M_left != 0)
    __y->_M_left->_M_parent = __x;
  __y->_M_parent = __x->_M_parent;

  if (__x == __root)
    __root = __y;
  else if (__x == __x->_M_parent->_M_left)
    __x->_M_parent->_M_left = __y;
  else
    __x->_M_parent->_M_right = __y;
  __y->_M_left = __x;
  __x->_M_parent = __y;
}

template <class _Dummy> void _STLP_CALL
_Rb_global<_Dummy>::_Rotate_right(_Rb_tree_node_base* __x,
                                  _Rb_tree_node_base*& __root) {
  _Rb_tree_node_base* __y = __x->_M_left;
  __x->_M_left = __y->_M_right;
  if (__y->_M_right != 0)
    __y->_M_right->_M_parent = __x;
  __y->_M_parent = __x->_M_parent;

  if (__x == __root)
    __root = __y;
  else if (__x == __x->_M_parent->_M_right)
    __x->_M_parent->_M_right = __y;
  else
    __x->_M_parent->_M_left = __y;
  __y->_M_right = __x;
  __x->_M_parent = __y;
}

template <class _Dummy> void _STLP_CALL
_Rb_global<_Dummy>::_Rebalance(_Rb_tree_node_base* __x,
                               _Rb_tree_node_base*& __root) {
  __x->_M_color = _S_rb_tree_red;
  while (__x != __root && __x->_M_parent->_M_color == _S_rb_tree_red) {
    if (__x->_M_parent == __x->_M_parent->_M_parent->_M_left) {
      _Rb_tree_node_base* __y = __x->_M_parent->_M_parent->_M_right;
      if (__y && __y->_M_color == _S_rb_tree_red) {
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __y->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        __x = __x->_M_parent->_M_parent;
      }
      else {
        if (__x == __x->_M_parent->_M_right) {
          __x = __x->_M_parent;
          _Rotate_left(__x, __root);
        }
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        _Rotate_right(__x->_M_parent->_M_parent, __root);
      }
    }
    else {
      _Rb_tree_node_base* __y = __x->_M_parent->_M_parent->_M_left;
      if (__y && __y->_M_color == _S_rb_tree_red) {
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __y->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        __x = __x->_M_parent->_M_parent;
      }
      else {
        if (__x == __x->_M_parent->_M_left) {
          __x = __x->_M_parent;
          _Rotate_right(__x, __root);
        }
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        _Rotate_left(__x->_M_parent->_M_parent, __root);
      }
    }
  }
  __root->_M_color = _S_rb_tree_black;
}

template <class _Dummy> _Rb_tree_node_base* _STLP_CALL
_Rb_global<_Dummy>::_Rebalance_for_erase(_Rb_tree_node_base* __z,
                                         _Rb_tree_node_base*& __root,
                                         _Rb_tree_node_base*& __leftmost,
                                         _Rb_tree_node_base*& __rightmost) {
  _Rb_tree_node_base* __y = __z;
  _Rb_tree_node_base* __x;
  _Rb_tree_node_base* __x_parent;

  if (__y->_M_left == 0)     
    __x = __y->_M_right;     
  else {
    if (__y->_M_right == 0)  
      __x = __y->_M_left;    
    else {                   
      __y = _Rb_tree_node_base::_S_minimum(__y->_M_right);   
      __x = __y->_M_right;
    }
  }

  if (__y != __z) {          
    __z->_M_left->_M_parent = __y;
    __y->_M_left = __z->_M_left;
    if (__y != __z->_M_right) {
      __x_parent = __y->_M_parent;
      if (__x) __x->_M_parent = __y->_M_parent;
      __y->_M_parent->_M_left = __x;      
      __y->_M_right = __z->_M_right;
      __z->_M_right->_M_parent = __y;
    }
    else
      __x_parent = __y;
    if (__root == __z)
      __root = __y;
    else if (__z->_M_parent->_M_left == __z)
      __z->_M_parent->_M_left = __y;
    else
      __z->_M_parent->_M_right = __y;
    __y->_M_parent = __z->_M_parent;
    _STLP_STD::swap(__y->_M_color, __z->_M_color);
    __y = __z;
    
  }
  else {                        
    __x_parent = __y->_M_parent;
    if (__x) __x->_M_parent = __y->_M_parent;
    if (__root == __z)
      __root = __x;
    else {
      if (__z->_M_parent->_M_left == __z)
        __z->_M_parent->_M_left = __x;
      else
        __z->_M_parent->_M_right = __x;
    }

    if (__leftmost == __z) {
      if (__z->_M_right == 0)        
        __leftmost = __z->_M_parent;
    
      else
        __leftmost = _Rb_tree_node_base::_S_minimum(__x);
    }
    if (__rightmost == __z) {
      if (__z->_M_left == 0)         
        __rightmost = __z->_M_parent;
    
      else                      
        __rightmost = _Rb_tree_node_base::_S_maximum(__x);
    }
  }

  if (__y->_M_color != _S_rb_tree_red) {
    while (__x != __root && (__x == 0 || __x->_M_color == _S_rb_tree_black))
      if (__x == __x_parent->_M_left) {
        _Rb_tree_node_base* __w = __x_parent->_M_right;
        if (__w->_M_color == _S_rb_tree_red) {
          __w->_M_color = _S_rb_tree_black;
          __x_parent->_M_color = _S_rb_tree_red;
          _Rotate_left(__x_parent, __root);
          __w = __x_parent->_M_right;
        }
        if ((__w->_M_left == 0 ||
             __w->_M_left->_M_color == _S_rb_tree_black) && (__w->_M_right == 0 ||
             __w->_M_right->_M_color == _S_rb_tree_black)) {
          __w->_M_color = _S_rb_tree_red;
          __x = __x_parent;
          __x_parent = __x_parent->_M_parent;
        } else {
          if (__w->_M_right == 0 ||
              __w->_M_right->_M_color == _S_rb_tree_black) {
            if (__w->_M_left) __w->_M_left->_M_color = _S_rb_tree_black;
            __w->_M_color = _S_rb_tree_red;
            _Rotate_right(__w, __root);
            __w = __x_parent->_M_right;
          }
          __w->_M_color = __x_parent->_M_color;
          __x_parent->_M_color = _S_rb_tree_black;
          if (__w->_M_right) __w->_M_right->_M_color = _S_rb_tree_black;
          _Rotate_left(__x_parent, __root);
          break;
        }
      } else {                  
        _Rb_tree_node_base* __w = __x_parent->_M_left;
        if (__w->_M_color == _S_rb_tree_red) {
          __w->_M_color = _S_rb_tree_black;
          __x_parent->_M_color = _S_rb_tree_red;
          _Rotate_right(__x_parent, __root);
          __w = __x_parent->_M_left;
        }
        if ((__w->_M_right == 0 ||
             __w->_M_right->_M_color == _S_rb_tree_black) && (__w->_M_left == 0 ||
             __w->_M_left->_M_color == _S_rb_tree_black)) {
          __w->_M_color = _S_rb_tree_red;
          __x = __x_parent;
          __x_parent = __x_parent->_M_parent;
        } else {
          if (__w->_M_left == 0 ||
              __w->_M_left->_M_color == _S_rb_tree_black) {
            if (__w->_M_right) __w->_M_right->_M_color = _S_rb_tree_black;
            __w->_M_color = _S_rb_tree_red;
            _Rotate_left(__w, __root);
            __w = __x_parent->_M_left;
          }
          __w->_M_color = __x_parent->_M_color;
          __x_parent->_M_color = _S_rb_tree_black;
          if (__w->_M_left) __w->_M_left->_M_color = _S_rb_tree_black;
          _Rotate_right(__x_parent, __root);
          break;
        }
      }
    if (__x) __x->_M_color = _S_rb_tree_black;
  }
  return __y;
}

template <class _Dummy> _Rb_tree_node_base* _STLP_CALL
_Rb_global<_Dummy>::_M_decrement(_Rb_tree_node_base* _M_node) {
  if (_M_node->_M_color == _S_rb_tree_red && _M_node->_M_parent->_M_parent == _M_node)
    _M_node = _M_node->_M_right;
  else if (_M_node->_M_left != 0) {
    _M_node = _Rb_tree_node_base::_S_maximum(_M_node->_M_left);
  }
  else {
    _Base_ptr __y = _M_node->_M_parent;
    while (_M_node == __y->_M_left) {
      _M_node = __y;
      __y = __y->_M_parent;
    }
    _M_node = __y;
  }
  return _M_node;
}

template <class _Dummy> _Rb_tree_node_base* _STLP_CALL
_Rb_global<_Dummy>::_M_increment(_Rb_tree_node_base* _M_node) {
  if (_M_node->_M_right != 0) {
    _M_node = _Rb_tree_node_base::_S_minimum(_M_node->_M_right);
  }
  else {
    _Base_ptr __y = _M_node->_M_parent;
    while (_M_node == __y->_M_right) {
      _M_node = __y;
      __y = __y->_M_parent;
    }
    
    
    
    if (_M_node->_M_right != __y)
      _M_node = __y;
  }
  return _M_node;
}

#endif 


template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>&
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::operator=(
  const _Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>& __x) {
  if (this != &__x) {
    
    clear();
    _M_node_count = 0;
    _M_key_compare = __x._M_key_compare;
    if (__x._M_root() == 0) {
      _M_root() = 0;
      _M_leftmost() = &this->_M_header._M_data;
      _M_rightmost() = &this->_M_header._M_data;
    }
    else {
      _M_root() = _M_copy(__x._M_root(), &this->_M_header._M_data);
      _M_leftmost() = _S_minimum(_M_root());
      _M_rightmost() = _S_maximum(_M_root());
      _M_node_count = __x._M_node_count;
    }
  }
  return *this;
}





template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::_M_insert(_Rb_tree_node_base * __parent,
                                                                      const _Value& __val,
                                                                      _Rb_tree_node_base * __on_left,
                                                                      _Rb_tree_node_base * __on_right) {
  
  
  _Base_ptr __new_node;

  if ( __parent == &this->_M_header._M_data ) {
    __new_node = _M_create_node(__val);
    _S_left(__parent) = __new_node;   
    _M_root() = __new_node;
    _M_rightmost() = __new_node;
  }
  else if ( __on_right == 0 &&     
           ( __on_left != 0 ||     
             _M_key_compare( _KeyOfValue()(__val), _S_key(__parent) ) ) ) {
    __new_node = _M_create_node(__val);
    _S_left(__parent) = __new_node;
    if (__parent == _M_leftmost())
      _M_leftmost() = __new_node;   
  }
  else {
    __new_node = _M_create_node(__val);
    _S_right(__parent) = __new_node;
    if (__parent == _M_rightmost())
      _M_rightmost() = __new_node;  
  }
  _S_parent(__new_node) = __parent;
  _Rb_global_inst::_Rebalance(__new_node, this->_M_header._M_data._M_parent);
  ++_M_node_count;
  return iterator(__new_node);
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_equal(const _Value& __val) {
  _Base_ptr __y = &this->_M_header._M_data;
  _Base_ptr __x = _M_root();
  while (__x != 0) {
    __y = __x;
    if (_M_key_compare(_KeyOfValue()(__val), _S_key(__x))) {
      __x = _S_left(__x);
    }
    else
      __x = _S_right(__x);
  }
  return _M_insert(__y, __val, __x);
}


template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
pair<__iterator__, bool>
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_unique(const _Value& __val) {
  _Base_ptr __y = &this->_M_header._M_data;
  _Base_ptr __x = _M_root();
  bool __comp = true;
  while (__x != 0) {
    __y = __x;
    __comp = _M_key_compare(_KeyOfValue()(__val), _S_key(__x));
    __x = __comp ? _S_left(__x) : _S_right(__x);
  }
  iterator __j = iterator(__y);
  if (__comp) {
    if (__j == begin())
      return pair<iterator,bool>(_M_insert(__y, __val,  __y), true);
    else
      --__j;
  }
  if (_M_key_compare(_S_key(__j._M_node), _KeyOfValue()(__val))) {
    return pair<iterator,bool>(_M_insert(__y, __val, __x), true);
  }
  return pair<iterator,bool>(__j, false);
}



template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_unique(iterator __position,
                                                                          const _Value& __val) {
  if (__position._M_node == this->_M_header._M_data._M_left) { 

    
    if (empty())
      return insert_unique(__val).first;

    if (_M_key_compare(_KeyOfValue()(__val), _S_key(__position._M_node))) {
      return _M_insert(__position._M_node, __val, __position._M_node);
    }
    
    else {
      bool __comp_pos_v = _M_key_compare( _S_key(__position._M_node), _KeyOfValue()(__val) );

      if (__comp_pos_v == false)  
        return __position;
      

      
      
      iterator __after = __position;
      ++__after;

      
      
      if (__after._M_node == &this->_M_header._M_data)
        
        
        
        return _M_insert(__position._M_node, __val, 0, __position._M_node);

      

      
      
      if (_M_key_compare( _KeyOfValue()(__val), _S_key(__after._M_node) )) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      }
      else {
        return insert_unique(__val).first;
      }
    }
  }
  else if (__position._M_node == &this->_M_header._M_data) { 
    if (_M_key_compare(_S_key(_M_rightmost()), _KeyOfValue()(__val))) {
        
        
        return _M_insert(_M_rightmost(), __val, 0, __position._M_node); 
    }
    else
      return insert_unique(__val).first;
  }
  else {
    iterator __before = __position;
    --__before;

    bool __comp_v_pos = _M_key_compare(_KeyOfValue()(__val), _S_key(__position._M_node));

    if (__comp_v_pos
        && _M_key_compare( _S_key(__before._M_node), _KeyOfValue()(__val) )) {

      if (_S_right(__before._M_node) == 0)
        return _M_insert(__before._M_node, __val, 0, __before._M_node); 
      else
        return _M_insert(__position._M_node, __val, __position._M_node);
      
    }
    else {
      
      iterator __after = __position;
      ++__after;
      
      bool __comp_pos_v = !__comp_v_pos;  
      
      
      
      
      if (!__comp_v_pos) {
        __comp_pos_v = _M_key_compare(_S_key(__position._M_node), _KeyOfValue()(__val));
      }

      if ( (!__comp_v_pos) 
          && __comp_pos_v
          && (__after._M_node == &this->_M_header._M_data ||
              _M_key_compare( _KeyOfValue()(__val), _S_key(__after._M_node) ))) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      } else {
        
        if (__comp_v_pos == __comp_pos_v)
          return __position;
        else
          return insert_unique(__val).first;
      }
    }
  }
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_equal(iterator __position,
                                                                         const _Value& __val) {
  if (__position._M_node == this->_M_header._M_data._M_left) { 

    
    if (size() <= 0)
        return insert_equal(__val);

    if (!_M_key_compare(_S_key(__position._M_node), _KeyOfValue()(__val)))
      return _M_insert(__position._M_node, __val, __position._M_node);
    else {
      
      if (__position._M_node->_M_left == __position._M_node)
        
        return _M_insert(__position._M_node, __val);

      
      
      
      iterator __after = __position;
      ++__after;

      
      
      
      
      if ( __after._M_node == &this->_M_header._M_data ||
           !_M_key_compare( _S_key(__after._M_node), _KeyOfValue()(__val) ) ) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      }
      else { 
        return insert_equal(__val);
      }
    }
  }
  else if (__position._M_node == &this->_M_header._M_data) { 
    if (!_M_key_compare(_KeyOfValue()(__val), _S_key(_M_rightmost())))
      return _M_insert(_M_rightmost(), __val, 0, __position._M_node); 
    else {
      return insert_equal(__val);
    }
  }
  else {
    iterator __before = __position;
    --__before;
    
    
    
    
    
    bool __comp_pos_v = _M_key_compare(_S_key(__position._M_node), _KeyOfValue()(__val));
    if (!__comp_pos_v &&
        !_M_key_compare(_KeyOfValue()(__val), _S_key(__before._M_node))) {
      if (_S_right(__before._M_node) == 0)
        return _M_insert(__before._M_node, __val, 0, __before._M_node); 
      else
        return _M_insert(__position._M_node, __val, __position._M_node);
    }
    else {
      
      
      iterator __after = __position;
      ++__after;

      if (__comp_pos_v &&
          ( __after._M_node == &this->_M_header._M_data ||
            !_M_key_compare( _S_key(__after._M_node), _KeyOfValue()(__val) ) ) ) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      }
      else { 
        return insert_equal(__val);
      }
    }
  }
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
_Rb_tree_node_base*
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::_M_copy(_Rb_tree_node_base* __x,
                                                                    _Rb_tree_node_base* __p) {
  
  _Base_ptr __top = _M_clone_node(__x);
  _S_parent(__top) = __p;

  _STLP_TRY {
    if (_S_right(__x))
      _S_right(__top) = _M_copy(_S_right(__x), __top);
    __p = __top;
    __x = _S_left(__x);

    while (__x != 0) {
      _Base_ptr __y = _M_clone_node(__x);
      _S_left(__p) = __y;
      _S_parent(__y) = __p;
      if (_S_right(__x))
        _S_right(__y) = _M_copy(_S_right(__x), __y);
      __p = __y;
      __x = _S_left(__x);
    }
  }
  _STLP_UNWIND(_M_erase(__top))

  return __top;
}


template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
void
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>::_M_erase(_Rb_tree_node_base *__x) {
  
  while (__x != 0) {
    _M_erase(_S_right(__x));
    _Base_ptr __y = _S_left(__x);
    _STLP_STD::_Destroy(&_S_value(__x));
    this->_M_header.deallocate(__STATIC_CAST(_Link_type, __x),1);
    __x = __y;
  }
}

#if defined (_STLP_DEBUG)
inline int
__black_count(_Rb_tree_node_base* __node, _Rb_tree_node_base* __root) {
  if (__node == 0)
    return 0;
  else {
    int __bc = __node->_M_color == _S_rb_tree_black ? 1 : 0;
    if (__node == __root)
      return __bc;
    else
      return __bc + __black_count(__node->_M_parent, __root);
  }
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
bool _Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>::__rb_verify() const {
  if (_M_node_count == 0 || begin() == end())
    return ((_M_node_count == 0) &&
            (begin() == end()) &&
            (this->_M_header._M_data._M_left == &this->_M_header._M_data) &&
            (this->_M_header._M_data._M_right == &this->_M_header._M_data));

  int __len = __black_count(_M_leftmost(), _M_root());
  for (const_iterator __it = begin(); __it != end(); ++__it) {
    _Base_ptr __x = __it._M_node;
    _Base_ptr __L = _S_left(__x);
    _Base_ptr __R = _S_right(__x);

    if (__x->_M_color == _S_rb_tree_red)
      if ((__L && __L->_M_color == _S_rb_tree_red) ||
          (__R && __R->_M_color == _S_rb_tree_red))
        return false;

    if (__L && _M_key_compare(_S_key(__x), _S_key(__L)))
      return false;
    if (__R && _M_key_compare(_S_key(__R), _S_key(__x)))
      return false;

    if (!__L && !__R && __black_count(__x, _M_root()) != __len)
      return false;
  }

  if (_M_leftmost() != _Rb_tree_node_base::_S_minimum(_M_root()))
    return false;
  if (_M_rightmost() != _Rb_tree_node_base::_S_maximum(_M_root()))
    return false;

  return true;
}
#endif 

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#undef _Rb_tree
#undef __iterator__
#undef iterator
#undef __size_type__

#endif 





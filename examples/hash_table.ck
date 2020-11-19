func newHashTable = function(void) obj {
  class HashTable(
    int size,
    func hash,
    func get,
    func set,
    func has,
    ref arr table,
    func keys,
    func values,
    func destroy
  );
  int _size = 1137;
  alloc arr _table = array() [_size] arr;
  return HashTable(
    _size,
    function(str string) int {
      int hash = 17;
      int i = 0;
      arr bytes = to_bytes(string);
      int bytes_length = size(bytes);
      for (; i < bytes_length; i += 1) {
        hash = (13 * hash * bytes[i]) % this.size;
      }
      return hash;
    },
    function(str key) str {
      int idx = this.hash(key);
      int _i = 0;
      for (; _i < size(this.table[idx]); _i += 1) {
        if (this.table[idx][_i][0] == key) {
          return this.table[idx][_i][1];
        }
      }
      return "";
    },
    function(str key, str val) void {
      int idx = this.hash(key);
      ref arr _tab = this.table;
      bool has = false;
      int _i = 0;
      for (; _i < size(_tab[idx]); _i += 1) {
        if (_tab[idx][_i][0] == key) {
          #_tab[idx][_i][1] = val;
          has = true;
        }
      }
      if (has) return;
      if (size(_tab[idx]) == 0) {
        #_tab[idx] = array(array(key, val) str) arr;
      } else {
        #_tab[idx] = _tab[idx] + array(key, val) str;
      }
    },
    function(str key) bool {
      int idx = this.hash(key);
      int _i = 0;
      for (; _i < size(this.table[idx]); _i += 1) {
        if (this.table[idx][_i][0] == key) return true;
      }
      return false;
    },
    _table,
    function(void) arr {
      arr _keys = array() str;
      int _i = 0;
      for (; _i < this.size; _i += 1) {
        int _j = 0;
        for (; _j < size(this.table[_i]); _j += 1) {
          _keys += this.table[_i][_j][0];
        }
      }
      return _keys;
    },
    function(void) arr {
      arr _values = array() str;
      int _i = 0;
      for (; _i < this.size; _i += 1) {
        int _j = 0;
        for (; _j < size(this.table[_i]); _j += 1) {
          _values += this.table[_i][_j][1];
        }
      }
      return _values;
    },
    function(void) void {
      del this.table;
    }
  );
};

obj table = newHashTable();

table.set("Key", "Value");
table.set("Hotel", "Trivago");

println("Key -> " + table.get("Key"));
table.set("Key", "Different value");
println("Key -> " + table.get("Key"));

println("Hotel? " + table.get("Hotel"));

println("Keys = " + to_str(table.keys()));
println("Values = " + to_str(table.values()));

println(table.has("bad"));
println(table.has("Key"));

table.destroy();
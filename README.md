![httq logo](https://github.com/wille-io/httq/blob/master/httq.png?raw=true)

# httq

Proof of Concept. Do not use in production.
HTTP Server for Qt. Doesn't clog the memory like The Other HTTP Server™.
Based on NodeJS's wonderful http-parser.
Can download a message's body in chunks, instead of reading it all into memory, allowing an easy DoS, like The Other HTTP Server™.

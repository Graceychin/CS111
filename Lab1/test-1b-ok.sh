#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
echo hi || rm abcdefg && echo dont print me
cat abcdefg || rm abcdefg && echo dont print me
cat abcdefg ||  echo print me

(echo aaaaaaaaaaaaaaaaaaaa)
EOF

cat >test.exp <<'EOF'
hi
dont print me
cat: abcdefg: No such file or directory
rm: cannot remove `abcdefg': No such file or directory
cat: abcdefg: No such file or directory
print me
aaaaaaaaaaaaaaaaaaaa
EOF

../timetrash test.sh >test.out 

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"

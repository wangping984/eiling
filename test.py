from pathlib import Path

print(Path('/root/dir/sub/onlyfile.ext').stem)


a = 'abc' + '.jpg'
print(a)

c = 'abc'
if c == 'abc':
    print('c is abc')
    quit()


b = 'frame' + '{:d}'.format(20)+'.jpg'
print(b)


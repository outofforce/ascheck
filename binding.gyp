{
  'targets': [
    {
      'target_name': 'asleveldb',
      'sources': [
        'addon/asleveldb.cc'
      ],
      'conditions': [
        ['OS=="linux"', {
          'include_dirs': [
            '/home/fanglf/src/leveldb-1.9.0/include '
          ],
          'libraries': [
            '-L/home/fanglf/src/leveldb-1.9.0 -lleveldb '
          ]
        }],
        ['OS=="mac"', {
          'include_dirs': [
            '/Users/outofforce/Work/src/leveldb-1.9.0/include '
          ],
          'libraries': [
            '-L/Users/outofforce/Work/src/leveldb-1.9.0 -lleveldb '
          ]
        }]
      ]
    }

  ]

}

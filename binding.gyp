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
        }]
      ]
    }

  ]

}

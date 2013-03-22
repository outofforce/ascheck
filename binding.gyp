{
  'targets': [
    {
      'target_name': 'zmq_addon',
      'sources': [
        'addon/zmq_addon.cc'
        './include'
      ],
      'conditions': [
        ['OS=="linux"', {
          'include_dirs': [
            '/home/fanglf/src/leveldb-1.9.0/include',
            '/usr/local/include',
            './include'
          ],
          'libraries': [
            '-L/home/fanglf/src/leveldb-1.9.0 -lleveldb '
            '-L/usr/local/lib -lzmq'
          ]
        }]
      ]
    },
    {
      'target_name': 'addon',
      'sources': [
        'addon/addon.cc'
      ],
      'conditions': [
        ['OS=="linux"', {
          'include_dirs': [
            '/home/fanglf/src/leveldb-1.9.0/include',
            './include'
          ],
          'libraries': [
            '-L/home/fanglf/src/leveldb-1.9.0 -lleveldb '
          ]
        }]
      ]
    }

  ]

}

import subprocess
import filecmp

num_tests = 3
passed_tests = 0

subprocess.run(['make', 'clean'])
subprocess.run(['make'])


for i in range(num_tests):
    print('\n')
    ref_test_path = 'refs/ref_{}.out'.format(i)
    test_path = 'tests/test_{}.out'.format(i)
    result_test_path = 'results/result_{}.out'.format(i)

    subprocess.run(['make', '''RUN_ARGS="{}" {}'''.format(test_path, 4), 'run'])

    test_passed = filecmp.cmp(ref_test_path, result_test_path)
    if (test_passed):
        passed_tests = passed_tests + 1
        print('test_{} ................................................ passed'.format(i))
    else:
        print('test_{} ................................................ failed - files differ'.format(i))

print('\nPassed {}/{} tests\n'.format(passed_tests, num_tests))
subprocess.run(['make', 'clean'])

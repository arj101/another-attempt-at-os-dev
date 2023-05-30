from build_utils import build

def main():#a
    build()
    # event_handler = PatternMatchingEventHandler(patterns=["*.py"], ignore_patterns=["*.o"], case_sensitive=True)
    #
    # event_handler.on_modified = on_file_change
    # observer = Observer()
    # observer.schedule(event_handler, ".", recursive=True)
    # observer.start()
    # try:
    #     while True:
    #         time.sleep(1)
    # except KeyboardInterrupt:
    #     print("\n[build.py] received interrupt, quitting\n")
        # observer.stop()
        # observer.join()

main()






def create_parser():
    from wptrunner import wptcommandline

    parser = wptcommandline.create_parser_update()
    parser.add_argument("--upstream", action="store_true",
                        help="Push local changes to upstream repository")
    parser.add_argument("--token-file", action="store", type=wptcommandline.abs_path,
                        help="Path to file containing github token")
    parser.add_argument("--token", action="store", help="GitHub token to use")
    return parser


def check_args(kwargs):
    from wptrunner import wptcommandline

    wptcommandline.set_from_config(kwargs)
    if kwargs["upstream"]:
        if kwargs["rev"]:
            raise ValueError("Setting --rev with --upstream isn't supported")
        if kwargs["token"] is None:
            if kwargs["token_file"] is None:
                raise ValueError("Must supply either a token file or a token")
            with open(kwargs["token_file"]) as f:
                token = f.read().strip()
                kwargs["token"] = token
    del kwargs["token_file"]
    return kwargs

def parse_args():
    parser = create_parser()
    kwargs = vars(parser.parse_args())
    return check_args(kwargs)

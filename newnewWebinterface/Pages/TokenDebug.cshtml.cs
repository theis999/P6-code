using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Security.Claims;

namespace RazorPageOidc.Pages;

[Authorize]
public class TokenDebugModel : PageModel
{
    public string? AccessToken { get; set; }
    public string? IdToken { get; set; }
    public string? userId { get; set; }
    public static string SubjectId(this ClaimsPrincipal user) { return user?.Claims?.FirstOrDefault(c => c.Type.Equals("sub", StringComparison.OrdinalIgnoreCase))?.Value; }


    public async Task OnGetAsync()
    {
        AccessToken = await HttpContext.GetTokenAsync("access_token");
        IdToken = await HttpContext.GetTokenAsync("id_token");
        userId = SubjectId;
    }
}